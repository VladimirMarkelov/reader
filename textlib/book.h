#ifndef BOOK_H_20161110_INCLUDED
#define BOOK_H_20161110_INCLUDED

#include <stdio.h>

#define READ_BUFF_SIZE 8192
#define MAX_BOOK_SIZE (1024*1024*30)

#define BOOK_LOAD_SUCCEES 0
#define BOOK_LOAD_FAILED 1
#define BOOK_PARSE_FAILED 2
#define BOOK_PARSE_INVALID_FORMAT 3
#define BOOK_PARSE_SUCCESS 100

#define BOOK_READ 1
#define BOOK_WRITE 0

#define MAX_ENC_STRING 32
#define PARA_INDENT 4

#ifdef __cplusplus
extern "C"
{
#endif

struct book_info {
    FILE *fd;
    int status;
    int options;
    int zipped;
    size_t text_sz;
    char encoding[MAX_ENC_STRING];
    char *author;
    char *title;
    char *filename;
    char *text;
};

int book_open(char* path, struct book_info *book, int reading);
void book_close(FILE* book);

struct pre_options {
    /* maximal text width */
    size_t width;
    /* disable hyphenation */
    int hyph_disable;
    /* add extra spaces between words to make string maximum width */
    int add_spaces;
};
struct book_preformat {
    char *line;
    size_t sz;
    size_t cap;
    size_t bt_sz;
    size_t indent;
    struct book_preformat* next;
    int attr;
};
struct book_preformat* book_preformat_mono(const struct book_info *book, struct pre_options *opts);
void book_preformat_free(struct book_preformat* fmt);

#ifdef __cplusplus
}
#endif

#endif // BOOK_H_20161110_INCLUDED
