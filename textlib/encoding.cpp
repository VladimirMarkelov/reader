#include <stdlib.h>
#include <string.h>
#include <iconv.h>
#include "encoding.h"

int detect_bom(const char *text, size_t sz) {
    if (text == NULL || *text == '\0' || sz < 2) {
        return 0;
    }

    if (sz >= 2) {
        if (text[0] == '\xFF' && text[1] == '\xFE') {
            return 2;
        } else if (text[0] == '\xFE' && text[1] == '\xFF') {
            return 2;
        }
    }
    if (sz > 2 && text[0] == '\xEF' && text[1] == '\xBB' && text[2] == '\xBF') {
        return 3;
    }

    return 0;
}

int detect_encoding(const char *text, size_t sz) {
    if (text == NULL || *text == '\0') {
        return ENC_UNKNOWN;
    }

    if (sz < 2) {
        return ENC_UNKNOWN;
    }

    if (sz >= 2) {
        if (text[0] == '\xFF' && text[1] == '\xFE') {
            return ENC_UTF16_LE;
        } else if (text[0] == '\xFE' && text[1] == '\xFF') {
            return ENC_UTF16_BE;
        }
    }
    if (sz > 2 && text[0] == '\xEF' && text[1] == '\xBB' && text[2] == '\xBF') {
        return ENC_UTF8;
    }

    int ru_enc = detect_ru_encoding(text, sz);

    if (ru_enc != ENC_UNKNOWN) {
        return ru_enc;
    }

    if (detect_utf8_encoding(text, sz)) {
        return ENC_UTF8;
    }

    return ENC_UNKNOWN;
}

int detect_ru_encoding(const char *text, size_t sz) {
    static int alt[/*128..255*/] = {1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,
            17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32, 1, 2, 3, 4, 5, 6,
            7, 8, 9,10,11,12,13,14,15,16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 17,18,19,20,21,22,23,24,
            25,26,27,28,29,30,31,32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0};
    static int win[/*128..255*/] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16, 17,18,19,
            20,21,22,23,24,25,26,27,28,29,30,31,32, 1, 2, 3, 4, 5, 6, 7, 8, 9,
            10,11,12,13,14,15,16, 17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32};
    static int koi[/*128..255*/] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 31, 1, 2,23, 5, 6,21, 4,22, 9,10,11,12,13,14,15, 16,32,17,
            18,19,20, 7, 3,29,28, 8,25,30,26,24,27, 31, 1, 2,23, 5, 6,21, 4,22,
            9,10,11,12,13,14,15, 16,32,17,18,19,20, 7, 3,29,28, 8,25,30,26,24,27};
    static int chardouble[32][32] = {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1},
        {1,1,1,1,1,1,1,0,1,0,1,1,1,1,1,0,1,1,1,1,0,1,1,1,1,1,1,1,1,0,1,1},
        {1,1,1,1,1,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,0,0,1},
        {1,1,1,1,1,1,0,0,1,0,1,1,1,1,1,0,1,1,1,1,0,0,0,1,1,0,0,0,0,0,0,0},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,0,1,1,1,0,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1},
        {1,1,0,1,1,1,1,0,1,0,1,1,1,1,1,1,1,1,0,1,0,0,0,1,0,0,0,0,1,0,0,0},
        {1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,0,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1},
        {0,0,0,1,1,1,0,1,0,0,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,1},
        {1,0,1,0,0,1,1,1,1,0,1,1,0,1,1,0,1,1,1,1,0,0,1,0,1,0,0,0,0,0,1,0},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,0,1,0,1,1,1,0,1,1,0,1,1},
        {1,1,1,1,1,1,0,0,1,0,1,1,1,1,1,1,1,1,0,1,0,1,1,1,1,0,0,1,1,1,0,1},
        {1,1,1,1,1,1,1,1,1,0,1,0,1,1,1,0,1,1,1,1,1,0,1,1,1,1,0,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1},
        {1,0,0,0,0,1,0,0,1,0,1,1,0,1,1,1,1,1,1,1,0,0,1,1,1,0,0,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,0,1,1},
        {1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,0,0,0,1,1,1},
        {1,0,0,0,0,1,0,0,1,0,0,1,1,1,1,0,1,1,1,1,0,0,0,0,0,0,0,1,0,0,1,0},
        {1,0,1,1,1,1,0,0,1,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,0,0,0,0,1,0,0},
        {1,0,1,0,0,1,0,0,1,0,1,1,1,0,1,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0},
        {1,0,1,0,0,1,0,0,1,0,1,1,1,1,1,0,1,0,1,1,1,0,0,0,1,0,0,0,1,0,0,0},
        {1,0,1,0,1,1,0,0,1,0,1,1,1,1,1,1,1,1,1,1,0,1,1,1,0,0,0,0,1,0,1,0},
        {1,0,0,0,0,1,0,0,1,0,0,0,0,1,1,0,1,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0},
        {0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
        {0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,0,0,0,0,0,1},
        {0,1,1,1,1,1,0,1,1,0,1,0,1,1,1,0,0,1,1,0,1,1,1,1,1,1,0,0,1,0,1,1},
        {0,0,0,1,0,1,0,1,0,0,0,1,1,1,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0},
        {0,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,0,0,1,1,1,1,1,0,0,0,0,1,0},
        {0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,0,0,0,0,1,1}
    };

    size_t alt_cnt = 0, win_cnt = 0, koi_cnt = 0,
        ascii_cnt = 0, non_ascii_cnt = 0;
    size_t prev = 128;

    size_t idx = 0;
    unsigned char *str = (unsigned char *)text;
    while (idx < sz) {
        if (*str < 128) {
            ascii_cnt++;
        } else {
            non_ascii_cnt++;
            size_t iswin = win[*str - 128];
            size_t isalt = alt[*str - 128];
            size_t iskoi = koi[*str - 128];
            if (iswin > 0) {
                if (prev < 128 && chardouble[prev][iswin]) {
                    win_cnt++;
                }
                prev = iswin;
            } else if (iskoi > 0) {
                if (prev < 128 && chardouble[prev][iskoi]) {
                    koi_cnt++;
                }
                prev = iskoi;
            } else if (isalt > 0) {
                if (prev < 128 && chardouble[prev][isalt]) {
                    alt_cnt++;
                }
                prev = isalt;
            }
        }
    }

    if (win_cnt > alt_cnt && win_cnt > koi_cnt && win_cnt > sz/10) {
        return ENC_WIN1251;
    } else if (alt_cnt > win_cnt && alt_cnt > koi_cnt && alt_cnt > sz/10) {
        return ENC_DOS866;
    } else if (alt_cnt > win_cnt && alt_cnt > koi_cnt && alt_cnt > sz/10) {
        return ENC_DOS866;
    }

    if (non_ascii_cnt == 0) {
        return ENC_ASCII;
    }

    return ENC_UNKNOWN;
}

int detect_utf8_encoding(const char *text, size_t sz) {
    unsigned char *str = (unsigned char *)text;
    size_t left = 0;
    while (sz > 0) {
        if (*str < 128) {
            if (left > 0) {
                return ENC_UNKNOWN;
            }

            str++;
            sz--;
        }

        if (left > 0) {
            if ((*str & 0xC0) == 0xC0) {
                left--;
                str++;
                sz--;
            } else {
                return ENC_UNKNOWN;
            }
        } else {
            int cnt = 0;
            unsigned char c = *str;
            while ((c & 0x80) == 0x80) {
                cnt++;
                c <<= 1;
            }
            if (c < 2 || c > 6) {
                return ENC_UNKNOWN;
            }

            left = cnt - 1;
            str++;
            sz--;
            if (sz < left) {
                break;
            }
        }
    }

    return ENC_UTF8;
}

const char* get_encoder_string(int encoder) {
    static const char* enclist[] = {
        "ASCII", "UTF-8", "UTF-16LE", "UTF-16BE",
        "CP1251", "KOI8-R", "CP866", "CP1252"
    };
    if (encoder < 0 || encoder >= ENC_WIN1252) {
        return NULL;
    }

    return enclist[encoder];
}

int convert_to_utf8(char **text, const char *enc) {
    if (text == NULL || enc == NULL || *text == NULL) {
        return 0;
    }

    size_t in_size = strlen(*text);
    size_t out_size = 4 * in_size;
    size_t max_size = out_size;
    char *out_text = (char *)malloc(out_size);

    iconv_t h = iconv_open("UTF-8", enc);
    char *inbuf = *text, *outbuf=out_text;
    size_t res = iconv(h, &inbuf, &in_size, &outbuf, &out_size);
    iconv_close(h);

    if (res == (size_t)-1) {
        free(out_text);
        return -1;
    }

    free(*text);
    size_t used = max_size - out_size;
    *text = (char *)malloc(sizeof(char) * (used + 1));
    strncpy(*text, out_text, used);
    (*text)[used] = '\0';
    free(out_text);

    return 0;
}

size_t convert_to_utf8_buffer(char *in, size_t in_sz, char *out, size_t out_sz, const char *enc) {
    if (in == NULL || in_sz == 0 || enc == NULL) {
        return -1;
    }

    if (strncmp("UTF-8", enc, 5) == 0 || strncmp("ASCII", enc, 5) == 0 ) {
        return 0;
    }

    if (out == NULL) {
        return 4 * in_sz;
    }

    size_t in_size = in_sz;
    size_t out_size = 4 * in_size;
    size_t max_size = out_sz;

    iconv_t h = iconv_open("UTF-8", enc);
    char *inbuf = in, *outbuf = out;
    size_t res = iconv(h, &inbuf, &in_size, &outbuf, &out_size);
    iconv_close(h);

    if (res == (size_t)-1) {
        return -2;
    }

    return max_size - out_size;
}

int convert_from_utf8(char **text, const char *enc) {
    if (text == NULL || enc == NULL || *text == NULL) {
        return 0;
    }

    size_t in_size = strlen(*text);
    size_t out_size = 2 * in_size;
    size_t max_size = out_size;
    char *out_text = (char *)malloc(out_size);

    iconv_t h = iconv_open(enc, "UTF-8");
    char *inbuf = *text, *outbuf=out_text;
    size_t res = iconv(h, &inbuf, &in_size, &outbuf, &out_size);
    iconv_close(h);

    if (res == (size_t)-1) {
        free(out_text);
        return -1;
    }

    free(*text);
    size_t used = max_size - out_size;
    *text = (char *)malloc(sizeof(char) * (used + 1));
    strncpy(*text, out_text, used);
    (*text)[used] = '\0';
    free(out_text);

    return 0;
}

