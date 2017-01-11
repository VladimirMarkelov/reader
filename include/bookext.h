#ifndef BOOKEXT_H_20161203
#define BOOKEXT_H_20161203

#include "book.h"

/* TEXT CODES */
#define TEXT_PART 1
#define TEXT_ATTR 2
#define TEXT_RESET 0xFF
#define TEXT_OFF 0x80

/* TEXT PARTS */
#define TEXT_SECTION 1
#define TEXT_PAGE 2
#define TEXT_PARA 3
#define TEXT_PICTURE 4
#define TEXT_EPIGRAPH 5
#define TEXT_CODE 6
#define TEXT_HDR1 7
#define TEXT_HDR2 8
#define TEXT_LIST 13
#define TEXT_LIST_ITEM 14
#define TEXT_AUTHOR 15
#define TEXT_TITLE 16

/* TEXT ATTRS */
#define TEXT_ATTR_STRONG 1
#define TEXT_ATTR_EM 2
#define TEXT_ATTR_ITALIC 3
#define TEXT_LINK 5
#define TEXT_ATTR_UPSCRIPT 5
#define TEXT_ATTR_SUBSCRIPT 6

/* TEXT PARSE OPTIONS */
#define BOOK_PARSE_EMPTY_LINE 1
#define BOOK_PARSE_INDENTED 2
/* 00 - every new line starts new paragraph
 * 01 - every paragraph is separated from other paragraph with empty line
 * 02 - every paragraph starts from indented line
 */

typedef int  (*fn_load_book) (struct book_info *info);
typedef void (*fn_unload_book) (const struct book_info *info);
typedef int  (*fn_supported) (const struct book_info *info);
typedef int  (*fn_enc) (struct book_info *info);

#endif // BOOKEXT_H_20161203
