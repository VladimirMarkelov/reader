#ifndef __MAIN_20161110_H__
#define __MAIN_20161110_H__

#include <windows.h>

/*  To use this exported function of dll, include this header
 *  in your project.
 */

#ifdef BUILD_DLL
    #define DLL_EXPORT __declspec(dllexport)
#else
    #define DLL_EXPORT __declspec(dllimport)
#endif


#ifdef __cplusplus
extern "C"
{
#endif

#define BOOK_HEADER "trz\x6F\x6F"

#define ENC_UNKNOWN -1
#define ENC_ASCII 0
#define ENC_UTF8 1
#define ENC_UTF16_LE 2
#define ENC_UTF16_BE 3
#define ENC_WIN1251 4
#define ENC_KOI8R 5
#define ENC_DOS866 6
#define ENC_ISO_8859_1 7

int DLL_EXPORT detect_encoding(const char *text, size_t sz);
int DLL_EXPORT detect_bom(const char *text, size_t sz);
int DLL_EXPORT detect_ru_encoding(const char *text, size_t sz);
int DLL_EXPORT detect_utf8_encoding(const char *text, size_t sz);
const char* DLL_EXPORT get_encoder_string(int encoder);

int DLL_EXPORT is_encoding_supported(const char *enc);
size_t DLL_EXPORT convert_to_utf8_buffer(char *in, size_t in_sz, char *out, size_t out_sz, const char *enc);
size_t DLL_EXPORT convert_from_utf8_buffer(char *in, size_t in_sz, char *out, size_t out_sz, const char *enc);

// TODO: convert defines to enums
#define BOOK_SUCCESS 0
#define BOOK_FAIL 1
#define BOOK_NO_MEMORY 2
#define BOOK_NO_PATH 3
#define BOOK_INVALID_ARG 4
#define BOOK_CONVERT_FAIL 5
#define BOOK_BUFFER_SMALL 6
#define BOOK_INVALID_UTF 7
#define BOOK_ENC_UNSUPPORTED 8
#define BOOK_INVALID_FILE 9
#define BOOK_NO_TEXT 10
#define BOOK_EQUAL 11
#define BOOK_NOT_EQUAL 12

#define BOOK_ITEM_NEW_LINE 13
#define BOOK_ITEM_META 14
#define BOOK_ITEM_TEXT 15

#define BOOK_PLAIN_TEXT 0
#define BOOK_ZIP 1
#define BOOK_EMPTY_ZIP 2
#define BOOK_UNKNOWN_FORMAT 127

#ifdef _WIN32
    #define DIR_SEP '\\'
#else
    #define DIR_SEP '/'
#endif

int DLL_EXPORT get_user_directory(char *dir, size_t len);
int DLL_EXPORT get_temp_directory(char *dir, size_t len);
int DLL_EXPORT append_path(char *dir, size_t dir_max, char *subdir);
int DLL_EXPORT path_up(char *dir);
int DLL_EXPORT to_upper(char *str, size_t sz);
int DLL_EXPORT to_lower(char *str, size_t sz);
int DLL_EXPORT make_relative(char *dir, char *main_dir);
int DLL_EXPORT path_is_absolute(char *dir);

// copyright sign as it is displayed by utf8proc
#define COPYRIGHT 9426
//#define COPYRIGHT 0x24d2

size_t DLL_EXPORT utf_len(char *str);
char* DLL_EXPORT utf_advance(char *str, size_t cnt);
int DLL_EXPORT utf_substr(char *str, size_t from, size_t to, char *dest, size_t dest_sz);
char* DLL_EXPORT utf_skip_spaces(char *str, size_t *count);
char* DLL_EXPORT utf_skip_newline(char *str);
char* DLL_EXPORT utf_next_line(char *str);
char* DLL_EXPORT utf_end_of_line(char *str);
size_t DLL_EXPORT utf_line_length(char *str, size_t *spaces);
char* DLL_EXPORT utf_word_forward(char *str, char** word_start);
char* DLL_EXPORT utf_line_forward(char *str, char** word_start);
int DLL_EXPORT utf_word_count(char *str, size_t maxch);
int DLL_EXPORT utf_starts_with(char *str, char *pattern);
int DLL_EXPORT utf_line_is_empty(char *str);
int DLL_EXPORT utf_is_first_char_lower(char *str);
int DLL_EXPORT utf_is_first_char_upper(char *str);
int DLL_EXPORT utf_make_wide(char *str, size_t buf_sz, size_t width);

int DLL_EXPORT hyphenation(char *str, size_t *hyph, size_t sz);

int DLL_EXPORT detect_zip(const char *text, size_t sz);
int DLL_EXPORT detect_book(const char *text, size_t sz);

#define EXT_BUF_NODE_SIZE 8192
struct ext_buffer_node_t {
    size_t used;
    struct ext_buffer_node_t* next;
    char *data;
};
struct ext_buffer_t {
    struct ext_buffer_node_t* head;
    struct ext_buffer_node_t* current;
};

struct ext_buffer_t* DLL_EXPORT ext_buffer_init();
int DLL_EXPORT ext_buffer_destroy(struct ext_buffer_t *buf);
int DLL_EXPORT ext_buffer_put_char(struct ext_buffer_t *buf, char c);
int DLL_EXPORT ext_buffer_put_string(struct ext_buffer_t *buf, char *str, size_t sz);
size_t DLL_EXPORT ext_buffer_size(const struct ext_buffer_t *buf);
int DLL_EXPORT ext_buffer_copy_data(const struct ext_buffer_t *buf, char *dest, size_t dest_sz);
char DLL_EXPORT ext_buffer_last_char(const struct ext_buffer_t *buf);

#ifdef __cplusplus
}
#endif

#endif // __MAIN_20161110_H__
