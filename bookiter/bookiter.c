#include "bookutil.h"
#include "bookiter.h"

#include "utf8proc.h"

struct book_iterator* iterator_init(struct book_info_t* book) {
    if (book == NULL || book->status != BOOK_PARSE_SUCCESS) {
        return NULL;
    }

    if (book->text == NULL || detect_book(book->text, book->text_sz) != BOOK_SUCCESS) {
        return NULL;
    }

    struct book_iterator *it = (struct book_iterator*)malloc(sizeof(struct book_iterator));

    it->book = book;
    it->pos = 0;
    it->save_pos = (size_t)-1;

    return it;
}

void iterator_free(struct book_iterator* it) {
    if (it) {
        it->book = NULL;
        free(it);
    }
}

static int iterator_save(struct book_iterator* it) {
    if (it == NULL || it->book == NULL) {
        return BOOK_INVALID_ARG;
    }

    it->save_pos = it->pos;
    return BOOK_SUCCESS;
}

static int iterator_restore(struct book_iterator* it) {
    if (it == NULL || it->book == NULL) {
        return BOOK_INVALID_ARG;
    }

    it->pos = it->save_pos;
    return BOOK_SUCCESS;
}

static int iterator_valid(struct book_iterator* it) {
    if (it->pos >= it->book->text_sz - 1) {
        return 0;
    }

    return 1;
}

int iterator_skip_meta(struct book_iterator* it) {
    if (it == NULL || it->book == NULL) {
        return BOOK_INVALID_ARG;
    }

    if (! iterator_valid(it)) {
        return BOOK_NO_TEXT;
    }

    unsigned char c = (unsigned char)it->book->text[it->pos];
    if (c < 0x0A) {
        it->pos += 2;
    } else if (c < ' ') {
        it->pos++;
    }

    return BOOK_SUCCESS;
}

int iterator_skip_whitespaces(struct book_iterator* it) {
    if (it == NULL || it->book == NULL) {
        return BOOK_INVALID_ARG;
    }

    if (! iterator_valid(it)) {
        return BOOK_NO_TEXT;
    }

    utf8proc_uint8_t *tmp = (utf8proc_uint8_t*)&it->book->text[it->pos];
    utf8proc_int32_t cp;
    size_t c_len;

    while (it->pos < it->book->text_sz - 1) {
        c_len = utf8proc_iterate(tmp, -1, &cp);
        utf8proc_category_t ctg = utf8proc_category(cp);

        if (cp != 0x09 && ctg != UTF8PROC_CATEGORY_ZS) {
            break;
        }

        tmp += c_len;
        it->pos += c_len;
    }

    return BOOK_SUCCESS;
}

int iterator_next(struct book_iterator* it, char* buf, size_t buf_sz, size_t *cnt) {
    if (it == NULL || it->book == NULL) {
        return BOOK_INVALID_ARG;
    }

    if (buf == NULL || buf_sz < 1) {
        return BOOK_NO_MEMORY;
    }

    iterator_skip_whitespaces(it);

    if (! iterator_valid(it)) {
        return BOOK_NO_TEXT;
    }

    if (cnt != NULL) {
        *cnt = 0;
    }

    unsigned char c = (unsigned char)it->book->text[it->pos];
    if (c == 0x0A) {
        it->pos++;
        *buf = '\0';
        return BOOK_NEW_LINE;
    }

    if (c < 0x0A) {
        *buf = c;

        it->pos++;
        if (! iterator_valid(it)) {
            return BOOK_INVALID_FILE;
        }
        if (buf_sz < 3) {
            return BOOK_NO_MEMORY;
        }

        buf[1] = it->book->text[it->pos];
        buf[2] = '\0';
        return BOOK_META;
    }

    if (c < 0x20) {
        return BOOK_INVALID_FILE;
    }

    --- copy till whitespace|0xA|meta ---
}

size_t iterator_max_width(struct book_iterator* it) {
    if (it == NULL || it->book == NULL) {
        return (size_t) -1;
    }

    if (! iterator_valid(it)) {
        return (size_t) -1;
    }

    size_t max_len = 0;
    size_t line_len;

    iterator_save(it);

    while (1) {
        line_len = iterator_line_len(it);
        if (line_len > max_len) {
            max_len = line_len;
        }

        iterator_end_line(it);
        if (it->book->text[it->pos] < 0x0A) {
            break;
        }

        iterator_skip_meta(it);
        if (! iterator_valid(it)) {
            break;
        }
    }

    iterator_restore(it);
    return max_len;
}

size_t iterator_line_len(struct book_iterator* it) {
    if (it == NULL || it->book == NULL) {
        return (size_t) -1;
    }

    if (! iterator_valid(it)) {
        return (size_t) -1;
    }

    utf8proc_uint8_t *tmp = (utf8proc_uint8_t*)&it->book->text[it->pos];
    utf8proc_int32_t cp;
    size_t c_len, line_len = 0;

    while (it->pos < it->book->text_sz - 1) {
        c_len = utf8proc_iterate(tmp, -1, &cp);

        if (cp < 0x20) {
            break;
        }

        tmp += c_len;
        line_len++;
    }

    return line_len;
}

int iterator_end_line(struct book_iterator* it) {
    if (it == NULL || it->book == NULL) {
        return BOOK_INVALID_ARG;
    }

    if (! iterator_valid(it)) {
        return BOOK_NO_TEXT;
    }

    utf8proc_uint8_t *tmp = (utf8proc_uint8_t*)&it->book->text[it->pos];
    utf8proc_int32_t cp;
    size_t c_len;

    while (it->pos < it->book->text_sz - 1) {
        c_len = utf8proc_iterate(tmp, -1, &cp);

        if (cp < 0x20) {
            break;
        }

        tmp += c_len;
        it->pos += c_len;
    }

    return BOOK_SUCCESS;
}

int iterator_next_line(struct book_iterator* it) {
    int res = iterator_end_line(it);

    if (res != BOOK_SUCCESS) {
        return res;
    }

    unsigned char c = (unsigned char)it->book->text[it->pos];
    if (c == 0x0A) {
        it->pos++;
    } else  if (c < 0x0A) {
        it->pos += 2;
    }

    return BOOK_SUCCESS;
}
