#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>

#include "utf8proc.h"
#include "book.h"
#include "bookutil.h"
#include "bookext.h"
#include "plaintext.h"
#include "bookiter.h"

#define FORMAT_BUF_SIZE 1024
#define MAX_HYPS 32

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

struct plugin_info {
    struct plugin_info *next;
    fn_load_book load;
    fn_enc enc;
    fn_supported supported;
    fn_unload_book unload;
    /* TODO: HANDLE to library to keep it loaded */
#ifdef _WIN32
    HANDLE handle;
#else
    void *handle;
#endif
};
static struct plugin_info *head = NULL;

void load_plugins() {
    /* TODO: look through folder 'plugins' and load all libraries
     * now it is just a stub for plain text. In real life plain text
     * processor must be the last in the list
     */

    head = (struct plugin_info*)malloc(sizeof(struct plugin_info));
    head->next = NULL;
    head->enc = get_encoding;
    head->load = prepare_book;
    head->supported = can_open;
    head->unload = free_book;
    head->handle = 0;
}

void unload_plugins() {
    struct plugin_info *tmp = head;
    while (tmp != NULL) {
        head = tmp;
        tmp = head->next;
        /* TODO: unload library */
        free(head);
    }
}


int book_open(char* path, struct book_info *book, int reading) {
    if (book == NULL) {
        return BOOK_LOAD_FAILED;
    }

    book->author = NULL;
    book->text = NULL;
    book->title = NULL;
    book->fd = NULL;
    book->status = BOOK_LOAD_FAILED;
    book->zipped = BOOK_PLAIN_TEXT;
    book->text_sz = 0;
    book->encoding[0] = '\0';

    iconv_t h = iconv_open("UTF-16LE", "UTF-8");

    char *wpath = (char *) calloc(sizeof(char), strlen(path) * 8);
    char *inpath = path;
    wchar_t *out_path = (wchar_t*)wpath;
    size_t in_left=strlen(path),
        out_left=strlen(path) * 8;

    iconv(h, &inpath, &in_left, &wpath, &out_left);

    const wchar_t *mode = (reading == BOOK_READ)? L"rb" : L"wb";
    book->fd = _wfopen(out_path, mode);

    iconv_close(h);
    free(out_path);

    book->status = BOOK_LOAD_SUCCEES;

    /* loop through all plugins and the first one replied that it can parse
     * the book gets book to make it readable
     */
    /* TODO: for the demo, there is no plugins, just default plain text */
    if (head == NULL) {
        load_plugins();
    }

    struct plugin_info* plugin = NULL;
    struct plugin_info* tmp = head;
    while (tmp != NULL) {
        if (tmp->supported(book) == BOOK_SUCCESS) {
            plugin = tmp;
            break;
        }

        tmp = tmp->next;
    }

    if (plugin == NULL) {
        printf("No plugin detected\n");
        book->status = BOOK_PARSE_INVALID_FORMAT;
    } else {
        printf("PLUGIN DETECTED\n");
        if (plugin->enc(book) != BOOK_SUCCESS) {
            printf("Failed to detect encoding\n");
        }
        printf("   enc == %s\n", book->encoding);
        plugin->load(book);
    }

    return book->status;
}

void book_close(struct book_info *book) {
    if (book == NULL) {
        return;
    }

    if (book->fd) {
        fclose(book->fd);
    }
    /*
    free(book->author);
    free(book->title);
    free(book->text);
    */
}

static struct book_preformat* new_preformat_line(struct book_preformat *parent, size_t max_width) {
    struct book_preformat *ln = (struct book_preformat*)malloc(sizeof(*ln));
    if (ln == NULL ) {
        return NULL;
    }

    ln->next = NULL;
    ln->line = (char *)malloc(sizeof(char) * (FORMAT_BUF_SIZE));
    if (ln->line == NULL) {
        free(ln);
        return NULL;
    }
    *(ln->line) = '\0';
    ln->indent = 0;
    ln->sz = 0;
    ln->bt_sz = 0;
    ln->cap = max_width;
    ln->attr = 0;

    if (parent != NULL) {
        parent->next = ln;
    }

    return ln;
}

static void line_fill_char(struct book_preformat *line, char c, size_t cnt) {
    while (line->sz < line->cap && cnt > 0) {
        line->line[line->bt_sz] = c;
        line->sz++;
        line->bt_sz++;
        cnt--;
    }
    line->line[line->bt_sz] = '\0';
}

struct book_preformat* book_preformat_mono(const struct book_info *book, struct pre_options *opts) {
    struct book_preformat* head = NULL;

    if (book == NULL || book->status != BOOK_PARSE_SUCCESS) {
        return head;
    }

    size_t max_width = (opts == NULL ? 80 : opts->width);
    if (max_width < 20 || max_width > 250) {
        return head;
    }

    head = new_preformat_line(NULL, max_width);
    if (head == NULL) {
        return head;
    }
    head->offset = 0;

    struct book_iterator *bit = iterator_init(book);
    if (bit == NULL) {
        return head;
    }

    int tp = BOOK_ITEM_TEXT;
    char buf[FORMAT_BUF_SIZE];
    size_t cnt, ind, width, slen;
    struct book_preformat* curr = head;
    int curr_section = 0,
        do_hyph = (opts == NULL ? 1 : !opts->hyph_disable),
        widen = (opts == NULL ? 0: opts->add_spaces);
    size_t hyps[MAX_HYPS];
    char *word;

    while (tp != BOOK_NO_TEXT) {
        tp = iterator_next(bit, buf, FORMAT_BUF_SIZE, &cnt);

        switch (tp) {
            case BOOK_ITEM_META:
                printf("META FOUND --------- \n");
                if (buf[0] == TEXT_PART) {
                    if ((buf[1] & TEXT_OFF) == TEXT_OFF) {
                        printf("META --- OFF \n");
                        curr_section = 0;
                        curr = new_preformat_line(curr, max_width);
                        curr->offset = bit->pos;

                        if ((buf[1] & 0x7F) == TEXT_TITLE) {
                            curr = new_preformat_line(curr, max_width);
                            curr->offset = bit->pos;
                        }

                        break;
                    }

                    switch (buf[1] & 0x7F) {
                        case TEXT_TITLE:
                            printf("TITLE found\n");
                            width = iterator_line_len(bit);
                            if (width < max_width) {
                                ind = (max_width - width) / 2;
                                line_fill_char(curr, ' ', ind);
                            }
                            curr_section = TEXT_TITLE;
                            curr->attr = TEXT_TITLE;
                            break;
                        case TEXT_EPIGRAPH:
                            printf("EPIGRAPH found\n");
                            width = iterator_section_max_width(bit);
                            if (width < max_width) {
                                ind = max_width - width;
                                line_fill_char(curr, ' ', ind);
                            }
                            curr_section = TEXT_EPIGRAPH;
                            curr->attr = TEXT_EPIGRAPH;
                            break;
                        case TEXT_PARA:
                            printf("PARAGRAPH found\n");
                            line_fill_char(curr, ' ', PARA_INDENT);
                            break;
                        default:
                            break;
                    }
                }
                break;
            case BOOK_ITEM_TEXT:
                printf("Line = [%s]\nNext word %d[%d + %d]: [%s]\n", curr->line, (int)strlen(buf), (int)cnt, (int)curr->sz, buf);

                word = buf;
                if (curr->cap <= curr->sz + cnt) {
                    if (do_hyph && curr->sz + 4 < curr->cap) {
                        printf("HYPH - try [%s]\n", buf);
                        int hypres = hyphenation(buf, hyps, MAX_HYPS);
                        if (hypres == BOOK_SUCCESS) {
                            size_t mx = curr->cap - curr->sz - 2;
                            size_t best = 0;
                            size_t j = 0;
                            while (hyps[j] > 0) {
                                if (hyps[j] <= mx) {
                                    best = hyps[j];
                                    j++;
                                } else {
                                    break;
                                }
                            }

                            if (best != 0) {
                                printf("Get first %d letters\n", (int)best);
                                strcat(curr->line, " ");
                                curr->bt_sz += 1;
                                curr->sz += 1;


                                utf8proc_uint8_t *src = (utf8proc_uint8_t*)buf;
                                utf8proc_uint8_t *dst = (utf8proc_uint8_t*)&curr->line[curr->bt_sz];
                                utf8proc_int32_t cp, last_char;
                                size_t counter = best, c_len;
                                while (counter > 0) {
                                    c_len = utf8proc_iterate(src, -1, &cp);
                                    utf8proc_encode_char(cp, dst);
                                    src += c_len;
                                    dst += c_len;
                                    curr->sz += 1;
                                    curr->bt_sz += c_len;
                                    last_char = cp;
                                    --counter;
                                }
                                *dst = '\0';
                                if (last_char != '-') {
                                    curr->sz += 1;
                                    curr->bt_sz += 1;
                                    strcpy((char *)dst, "-");
                                }

                                printf("Line NOW: [%s]\n", curr->line);
                                word = (char *)src;
                                cnt -= best;
                                printf("Word the rest [%s]\n", word);
                            }
                        }
                    }

                    if (widen && curr->attr != TEXT_TITLE && curr->attr != TEXT_EPIGRAPH) {
                        utf_make_wide(curr->line, FORMAT_BUF_SIZE, max_width);
                    }

                    curr = new_preformat_line(curr, max_width);
                    curr->offset = bit->pos;
                }

                if (cnt > max_width) {
                    printf("Too long word that cannot be hyphenated [%s], skipped", word);
                    break;
                }

                printf("Add to current line: %d   <--> %d - %d[%d]\n", (int)cnt, (int)curr->cap, (int)curr->sz, (int)curr->bt_sz);
                slen = strlen(word);
                if (slen + curr->bt_sz >= FORMAT_BUF_SIZE - 1) {
                    printf("Buffer (used %d bytes) too small to keep %d characters (req: %d bytes)\n", (int)curr->bt_sz, (int)max_width, (int)slen);
                    return head;
                }
                if (curr->sz != 0 && curr->line[curr->bt_sz - 1] != ' ') {
                    strcat(curr->line, " ");
                    curr->bt_sz += 1;
                    curr->sz += 1;
                }
                strcat(curr->line, word);
                curr->bt_sz += slen;
                curr->sz += cnt;
                printf("LINE %d - %d [%s]\n", (int)curr->sz, (int)curr->bt_sz, curr->line);

                break;
            case BOOK_NO_TEXT:
                /* do nothing - end of text reached */
                printf("BOOK FORMAT COMPLETED\n");
                break;
            case BOOK_ITEM_NEW_LINE:
                if (curr_section == TEXT_EPIGRAPH) {
                    curr = new_preformat_line(curr, max_width);
                    if (curr == NULL) {
                        return head;
                    }
                    curr->offset = bit->pos;
                    curr->attr = TEXT_EPIGRAPH;
                    line_fill_char(curr, ' ', ind);
                }
                break;
            case BOOK_BUFFER_SMALL:
                printf("BUFFER TOO SMALL\n");
                return head;
            default:
                printf("Invalid item type: %d\n", tp);
        }
    }

    return head;
}

void book_preformat_free(struct book_preformat* fmt) {
    struct book_preformat *head = fmt, *tmp;
    while (head != NULL) {
        free(head->line);
        tmp = head->next;
        free(head);
        head = tmp;
    }
}
