#ifndef PLAINTEXT_H_20161204
#define PLAINTEXT_H_20161204

#include "bookext.h"

#ifdef __cplusplus
extern "C"
{
#endif


int can_open (const struct book_info_t *info);
int get_encoding (struct book_info_t *info);
int prepare_book (struct book_info_t *info);
void free_book (const struct book_info_t *info);

#ifdef __cplusplus
}
#endif


#endif // PLAINTEXT_H_20161204
