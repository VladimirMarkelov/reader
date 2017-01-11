#ifndef BOOKITER_H_20161231
#define BOOKITER_H_20161231

#include "bookext.h"
#include "bookiter.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct book_iterator {
    const struct book_info *book;
    size_t pos;
    size_t save_pos;
};

struct book_iterator* iterator_init(const struct book_info* book);
void iterator_free(struct book_iterator* it);

int iterator_skip_meta(struct book_iterator* it);
int iterator_skip_whitespaces(struct book_iterator* it);

int iterator_next(struct book_iterator* it, char* buf, size_t buf_sz, size_t *cnt);

size_t iterator_section_max_width(struct book_iterator* it);
size_t iterator_line_len(struct book_iterator* it);

int iterator_end_line(struct book_iterator* it);
int iterator_next_line(struct book_iterator* it);

#ifdef __cplusplus
}
#endif


#endif // BOOKITER_H_20161231
