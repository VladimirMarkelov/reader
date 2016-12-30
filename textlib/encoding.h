#ifndef ENCODING_H_20161108
#define ENCODING_H_20161108

#define ENC_UNKNOWN -1
#define ENC_ASCII 0
#define ENC_UTF8 1
#define ENC_UTF16_LE 2
#define ENC_UTF16_BE 3
#define ENC_WIN1251 4
#define ENC_KOI8R 5
#define ENC_DOS866 6
#define ENC_WIN1252 7

int detect_encoding(const char *text, size_t sz);
int detect_bom(const char *text, size_t sz);
int detect_ru_encoding(const char *text, size_t sz);
int detect_utf8_encoding(const char *text, size_t sz);
const char* get_encoder_string(int encoder);

int convert_to_utf8(char **text, const char *enc);
int convert_from_utf8(char **text, const char *enc);
size_t convert_to_utf8_buffer(char *in, size_t in_sz, char *out, size_t out_sz, const char *enc);

#endif // ENCODING_H_20161108
