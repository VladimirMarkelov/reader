#include "plaintext.h"

#include "bookutil.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_LINES_CHECK 200
#define MAX_LINE_LEN 200
#define ENDLN 0x0A

static size_t detect_text_width(char *text);

static int is_book_valid(const struct book_info_t *info) {
    return info &&
           info->fd &&
           info->status == BOOK_LOAD_SUCCEES;
}

int get_encoding (struct book_info_t *info) {
    if (info == NULL) {
        return BOOK_INVALID_ARG;
    }
    if (info->fd == NULL) {
        return BOOK_INVALID_FILE;
    }

    /* encoding is already defined */
    if (*info->encoding != '\0') {
        return BOOK_SUCCESS;
    }

    rewind(info->fd);
    char buf[8192];

    size_t rd = fread(buf, sizeof(char), 8192, info->fd);

    int zip = detect_zip(buf, rd);
    if (zip == BOOK_EMPTY_ZIP) {
        strcpy(info->encoding, "UTF-8");
        info->zipped = BOOK_EMPTY_ZIP;
        return BOOK_SUCCESS;
    }

    if (zip == BOOK_ZIP) {
        info->zipped = BOOK_ZIP;
        /* TODO: unpack zip */
    }

    int enc = detect_encoding(buf, rd);

    if (enc == ENC_UNKNOWN) {
        return BOOK_FAIL;
    }

    const char *enc_str = get_encoder_string(enc);
    if (enc_str != NULL) {
        strncpy(info->encoding, enc_str, MAX_ENC_STRING);
        return BOOK_SUCCESS;
    }

    return BOOK_FAIL;
}

static int start_tag(struct ext_buffer_t* booktext, char section) {
    if (section < 1 || section > 0x7e) {
        return BOOK_INVALID_ARG;
    }

    ext_buffer_put_char(booktext, TEXT_PART);
    ext_buffer_put_char(booktext, section);

    return BOOK_SUCCESS;
}

static int close_tag(int *text_type, struct ext_buffer_t* booktext, int tag) {
    if (text_type == NULL) {
        if (tag == -1) {
            return BOOK_INVALID_ARG;
        }
        ext_buffer_put_char(booktext, TEXT_PART);
        ext_buffer_put_char(booktext, tag | TEXT_OFF);

        return BOOK_SUCCESS;
    }

    if (tag != -1 && *text_type != tag) {
        return BOOK_SUCCESS;
    }

    if (tag == -1 && *text_type == TEXT_OFF) {
        return BOOK_SUCCESS;
    }

    ext_buffer_put_char(booktext, TEXT_PART);
    ext_buffer_put_char(booktext, (tag == -1 ? *text_type : tag) | TEXT_OFF);

    if (*text_type == TEXT_PARA) {
        ext_buffer_put_char(booktext, ENDLN);
    }

    *text_type = TEXT_OFF;

    return BOOK_SUCCESS;
}

int prepare_book (struct book_info_t *info) {
    if (! is_book_valid(info)) {
        return BOOK_INVALID_ARG;
    }

    printf("Start loading book....\n");
    if (info->zipped == BOOK_EMPTY_ZIP) {
        info->status = BOOK_PARSE_SUCCESS;
        return BOOK_NO_TEXT;
    }
    if (! is_encoding_supported(info->encoding)) {
        info->status = BOOK_PARSE_FAILED;
        return BOOK_ENC_UNSUPPORTED;
    }

    size_t fsize = 0;
    char *tmpbuf = NULL;
    if (info->zipped != BOOK_PLAIN_TEXT) {
        printf("... prepare book ... zipped\n");
        /* TODO: unzip file */
    } else {
        /* TODO: load text as is */
        rewind(info->fd);
        fseek(info->fd, 0L, SEEK_END);
        fsize = ftell(info->fd);
        rewind(info->fd);
        tmpbuf = malloc(sizeof(char) * (fsize + 1));
        if (tmpbuf == NULL) {
            info->status = BOOK_PARSE_FAILED;
            return BOOK_NO_MEMORY;
        }
        fread(tmpbuf, sizeof(char), fsize, info->fd);
        tmpbuf[fsize] = '\0';
        printf("  plain text of size: %d\n", (int)fsize);
    }

    printf("   encoding %s\n", info->encoding);
    if (strcmp(info->encoding,"ASCII") != 0 &&
        strcmp(info->encoding, "UTF-8") != 0) {
        /* TODO: convert to UTF-8 */
        size_t usize = fsize * 4;
        char *utfbuf = (char *)malloc(usize);
        size_t ulen = convert_to_utf8_buffer(tmpbuf, fsize, utfbuf, usize, info->encoding);
        free(tmpbuf);
        tmpbuf = utfbuf;
        tmpbuf[ulen] = '\0';
    }

    /* parse book */
    size_t w = detect_text_width(tmpbuf);
    printf("   text width %d\n", (int)w);

    struct ext_buffer_t* booktext = ext_buffer_init();
    if (booktext == NULL) {
        free(tmpbuf);
        info->status = BOOK_PARSE_FAILED;
        return BOOK_NO_MEMORY;
    }

    int buferr = ext_buffer_put_string(booktext, BOOK_HEADER, 0);
    if (buferr != BOOK_SUCCESS) {
        ext_buffer_destroy(booktext);
        free(tmpbuf);
        info->status = BOOK_PARSE_FAILED;
        return buferr;
    }

    start_tag(booktext, TEXT_SECTION);
    size_t space_cnt, line_len;
    int last_type = TEXT_OFF, inside_para = 0, is_epi, is_title;
    int any_paragraph = 0;
    char *ptr = tmpbuf;
    char *line_end, *no_sp;
    char last_ch;

    while (*ptr != '\0') {
        printf(" ... next loop - [%d]\n", (int)*ptr);
        no_sp = utf_skip_spaces(ptr, &space_cnt);
        line_len = utf_line_length(no_sp, NULL);
        printf("..next line: %d spaces - %p\n", (int)space_cnt, no_sp);

        if (*no_sp == '\0' || *no_sp == '\x0D' || *no_sp == '\x0A') {
            printf("    empty line\n");
            ext_buffer_put_char(booktext, ENDLN);
            close_tag(&last_type, booktext, -1);
            start_tag(booktext, TEXT_PARA);
            close_tag(NULL, booktext, TEXT_PARA);
            ptr = utf_next_line(no_sp);
            continue;
        }

        int is_upcase = utf_is_first_char_upper(no_sp);
        is_title = is_upcase &&
                   space_cnt < 6 &&
                   line_len < 70*2/3 &&
                   utf_starts_with(no_sp, "-") != BOOK_EQUAL;

        if (space_cnt > 6 || (is_title && any_paragraph == 0)) {
            /* epigraph or title */
            if (inside_para) {
                close_tag(NULL, booktext, TEXT_PARA);
                last_type = TEXT_OFF;
                inside_para = 0;
            }

            is_epi = (w > 0 && (abs(w - space_cnt - line_len) < 10 && space_cnt > w / 4)) ||
                     (space_cnt > w / 4 && last_type == TEXT_EPIGRAPH) ||
                     (w == 0 && space_cnt > 35);

            if (is_epi) {
                /* epigraph */
                if (last_type != TEXT_EPIGRAPH) {
                    /* epigraph starts */
                    start_tag(booktext, TEXT_EPIGRAPH);
                    line_end = utf_end_of_line(no_sp);
                    ext_buffer_put_string(booktext, no_sp, line_end - no_sp);
                    ext_buffer_put_char(booktext, ENDLN);
                    ptr = utf_skip_newline(line_end);
                    last_type = TEXT_EPIGRAPH;
                } else {
                    /* epigraph continues */
                    line_end = utf_end_of_line(no_sp);
                    ext_buffer_put_string(booktext, no_sp, line_end - no_sp);
                    ext_buffer_put_char(booktext, ENDLN);
                    ptr = utf_skip_newline(line_end);
                }
            } else {
                /* title - for simplicity multi-line titles are treated as
                 * a few separate titles
                 */
                printf("Title detected\n");
                start_tag(booktext, TEXT_TITLE);
                line_end = utf_end_of_line(no_sp);
                ext_buffer_put_string(booktext, no_sp, line_end - no_sp);
                ext_buffer_put_char(booktext, ENDLN);
                close_tag(NULL, booktext, TEXT_TITLE);
                ptr = utf_skip_newline(line_end);
                last_type = TEXT_OFF;
            }
        } else {
            /* simple text */
            printf("Simple Paragraph Text\n");
            close_tag(&last_type, booktext, TEXT_EPIGRAPH);
            any_paragraph = 1;

            if (w == 0) {
                /* every new line is new paragraph */
                start_tag(booktext, TEXT_PARA);
                line_end = utf_end_of_line(no_sp);
                ext_buffer_put_string(booktext, no_sp, line_end - no_sp);
                ext_buffer_put_char(booktext, ENDLN);
                close_tag(NULL, booktext, TEXT_PARA);
            } else {
                if (space_cnt > 6 ||
                    ! inside_para ||
                    (space_cnt + line_len < w * 4 / 5 && is_upcase)
                    ) {
                    /* paragraph starts with the line that has >=2
                     * leading spaces or after non-paragraph section
                     */
                    printf("  new paragraph\n");
                    close_tag(&last_type, booktext, TEXT_PARA);
                    start_tag(booktext, TEXT_PARA);
                    last_type = TEXT_PARA;
                } else if (*no_sp != '\0' && utf_starts_with(no_sp, "-") == BOOK_EQUAL) {
                    /* direct speech */
                    printf("  direct speech\n");
                    close_tag(&last_type, booktext, TEXT_PARA);
                    start_tag(booktext, TEXT_PARA);
                    last_type = TEXT_PARA;
                }

                last_ch = ext_buffer_last_char(booktext);
                if (last_ch != '\0' && (unsigned int)last_ch > 0x20 && last_ch != '-') {
                    ext_buffer_put_char(booktext, ' ');
                }

                /*printf(" .. put text to buffer: %c\n", *no_sp);*/
                line_end = utf_end_of_line(no_sp);
                /* * /
                char aaa[1024];
                strncpy(aaa, no_sp, line_end - no_sp);
                aaa[line_end - no_sp] = '\0';
                printf(" .. put text to buffer end: [%d][%s]\n", (int)(line_end - no_sp), aaa);
                / * */
                ext_buffer_put_string(booktext, no_sp, line_end - no_sp);
                inside_para = 1;
            }

            ptr = utf_skip_newline(line_end);
            printf("... line processed: [%d] [%c]\n", (int)*ptr, *ptr);
        }
    }

    close_tag(&last_type, booktext, TEXT_PARA);
    printf("Book processed!\n");
    close_tag(NULL, booktext, TEXT_SECTION);
    printf("Book closed\n");

    size_t bsz = ext_buffer_size(booktext);
    printf("Buffer size: %d\n", (int)bsz);
    info->text_sz = bsz;
    info->text = (char*)malloc(bsz + 1);

    if (info->text == NULL) {
        ext_buffer_destroy(booktext);
        free(tmpbuf);
        info->status = BOOK_PARSE_FAILED;
        return BOOK_NO_MEMORY;
    }

    printf("Copying data to book buffer\n");
    ext_buffer_copy_data(booktext, info->text, bsz + 1);
    ext_buffer_destroy(booktext);
    free(tmpbuf);
    info->status = BOOK_PARSE_SUCCESS;

    return BOOK_SUCCESS;
}

int can_open (const struct book_info_t *info) {
    if (! is_book_valid(info)) {
        return BOOK_INVALID_ARG;
    }

    return BOOK_SUCCESS;
}

void free_book (const struct book_info_t *info) {
    if (info == NULL) {
        return;
    }

    if (info->author != NULL) {
        free(info->author);
    }
    if (info->title != NULL) {
        free(info->title);
    }
    if (info->text != NULL) {
        free(info->text);
    }
}

static size_t detect_text_width(char *text) {
    if (text == NULL) {
        return 0;
    }

    char *tmp = text;
    size_t lens[MAX_LINE_LEN / 10];
    size_t array_size = MAX_LINE_LEN / 10;
    memset(lens, 0, sizeof(size_t) * array_size);

    int to_check = MAX_LINES_CHECK;
    int empty = 0;
    printf("Counting line lengths\n");
    while (to_check && *tmp != '\0') {
        printf("... next line ... \n");
        if (utf_line_is_empty(tmp)) {
            printf(" ... empty line skipped\n");
            empty++;
            to_check--;
            tmp = utf_next_line(tmp);
            continue;
        }

        /*
        char aaa[17];
        strncpy(aaa, tmp, 17);
        aaa[16] = '\0';
        printf("---> %s <---\n", aaa);
        */
        printf("... not empty ... counting chars ... \n");
        size_t len = utf_line_length(tmp, NULL);
        printf(" ... line %d chars\n", (int)len);
        if (len > MAX_LINE_LEN) {
            /* it seems long text paragraphs are separated with LF,
             * so there is no right margin
             */
            return 0;
        }
        int m = len % 10;
        len /= 10;
        if (m > 0) {
            len++;
        }

        if (len < array_size) {
            lens[len]++;
        }

        tmp = utf_next_line(tmp);
        to_check--;
    }

    /* find the most frequent line length */
    size_t mx = lens[0];
    size_t linelen = 0;
    for (size_t i = 1; i < array_size; i++) {
        //printf(" ----> %d : %d <--------\n", (int)i, (int)lens[i]);
        if (lens[i] >= mx) {
            mx = lens[i];
            linelen = i * 10;
        }
    }

    /* if any length of non-empty lines is more frquent than 15% of the text
     * then use it as a text width
     */
    if (mx * 100 / (MAX_LINES_CHECK - to_check - empty) > 15) {
        return linelen;
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
