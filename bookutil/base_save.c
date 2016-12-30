#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>

#include "base.h"
#include "utf8proc.h"

typedef utf8proc_int32_t (*utf_func) (utf8proc_int32_t);

static int is_sep(char c) {
#ifdef _WIN32
    return c == DIR_SEP || c == '/';
#else
    return c == DIR_SEP;
#endif
}

static int path_has_root(char *dir) {
    if (dir == NULL || *dir == '\0') {
        return 0;
    }

#ifdef _WIN32
    return ((dir[0] >= 'A' && dir[0] <= 'Z') || (dir[0] >= 'a' && dir[0] <= 'z'))
           && dir[1] == ':'
           && (dir[2] == '\0' || is_sep(dir[2]));
#else
    return dir[0] == '/' && dir[1] == '\0';
#endif
}

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

    int enc = detect_utf8_encoding(text, sz);
    if (enc != ENC_UNKNOWN) {
        return enc;
    }

    enc = detect_ru_encoding(text, sz);
    if (enc != ENC_UNKNOWN) {
        return enc;
    }

    return ENC_ISO_8859_1;
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
           non_ascii_cnt = 0;
    size_t prev = 128;

    size_t cnts[3] = {0, 0, 0};
    size_t prevs[3] = {0, 0, 0};
    int *maps[3] = {win, alt, koi};

    for (int en = 0; en < 3; en++) {
        size_t idx = 0;
        unsigned char *str = (unsigned char *)text;

        while (idx < sz && *str != '\0') {
            if (*str < 128) {
                prev = 128;
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
                }
                if (iskoi > 0) {
                    if (prev < 128 && chardouble[prev][iskoi]) {
                        koi_cnt++;
                    }
                    prev = iskoi;
                }
                if (isalt > 0) {
                    if (prev < 128 && chardouble[prev][isalt]) {
                        alt_cnt++;
                    }
                    prev = isalt;
                }

                if (! iskoi && ! iswin && ! isalt) {
                    prev = 128;
                }
            }
            idx++;
            str++;
        }

        if (non_ascii_cnt == 0) {
            return ENC_ASCII;
        }
    }

    if (win_cnt > alt_cnt && win_cnt > koi_cnt && win_cnt > sz/10) {
        return ENC_WIN1251;
    } else if (alt_cnt > win_cnt && alt_cnt > koi_cnt && alt_cnt > sz/10) {
        return ENC_DOS866;
    } else if (koi_cnt > win_cnt && koi_cnt > alt_cnt && koi_cnt > sz/10) {
        return ENC_KOI8R;
    }

    return ENC_UNKNOWN;
}

int detect_utf8_encoding(const char *text, size_t sz) {
    unsigned char *str = (unsigned char *)text;
    size_t left = 0;
    int non_ascii_cnt = 0;

    while (sz > 0 && *str != '\0') {
        if (*str < 128) {
            if (left > 0) {
                return ENC_UNKNOWN;
            }

            str++;
            sz--;
            continue;
        } else {
            non_ascii_cnt++;
        }

        if (left > 0) {
            if ((*str & 0xC0) == 0x80) {
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
            if (cnt < 2 || cnt > 6) {
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

    return ((non_ascii_cnt > 0) ? ENC_UTF8 : ENC_ASCII);
}

const char* get_encoder_string(int encoder) {
    static const char* enclist[] = {
        "ASCII", "UTF-8", "UTF-16LE", "UTF-16BE",
        "CP1251", "KOI8-R", "CP866", "ISO-8859-1"
    };
    if (encoder < 0 || encoder >= ENC_ISO_8859_1) {
        return NULL;
    }

    return enclist[encoder];
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

size_t convert_from_utf8_buffer(char *in, size_t in_sz, char *out, size_t out_sz, const char *enc) {
    if (in == NULL || in_sz == 0 || enc == NULL) {
        return -1;
    }

    if (strncmp("UTF-8", enc, 5) == 0 || strncmp("ASCII", enc, 5) == 0 ) {
        return 0;
    }

    if (out == NULL) {
        return 2 * in_sz;
    }

    size_t in_size = in_sz;
    size_t out_size = 2 * in_size;
    size_t max_size = out_sz;

    iconv_t h = iconv_open(enc, "UTF-8");
    char *inbuf = in, *outbuf = out;
    size_t res = iconv(h, &inbuf, &in_size, &outbuf, &out_size);
    iconv_close(h);

    if (res == (size_t)-1) {
        return -2;
    }

    return max_size - out_size;
}

int DLL_EXPORT get_user_directory(char *dir, size_t len) {
    if (dir == NULL) {
        return BOOK_INVALID_ARG;
    }

#ifdef _WIN32
    const wchar_t *path = _wgetenv(L"APPDATA");
    iconv_t h = iconv_open("UTF-8", "UTF-16LE");

    char *outbuf = dir;
    char *inbuf = (char *)path;
    size_t insize = (wcslen(path) + 1) * sizeof(wchar_t);

    errno = 0;
    size_t res = iconv(h, &inbuf, &insize, &outbuf, &len);
    iconv_close(h);

    if (res == (size_t)-1) {
        return (errno == E2BIG) ? BOOK_BUFFER_SMALL : BOOK_CONVERT_FAIL;
    }
#else
#endif

    return BOOK_SUCCESS;
}

int DLL_EXPORT append_path(char *dir, size_t dir_max, char *subdir) {
    if (dir == NULL) {
        return BOOK_INVALID_ARG;
    }

    if (subdir == NULL || *subdir == '\0') {
        return BOOK_SUCCESS;
    }

    if (*dir == 0) {
        return BOOK_NO_PATH;
    }

    size_t dir_len = strlen(dir);
    if (dir_len + strlen(subdir) + 2 >= dir_max) {
        return BOOK_BUFFER_SMALL;
    }

    char last = dir[dir_len - 1];
    if (! is_sep(last)) {
        dir[dir_len] = DIR_SEP;
        dir[dir_len + 1] = '\0';
    }

    strcat(dir, subdir);

    return BOOK_SUCCESS;
}

int DLL_EXPORT path_up(char *dir) {
    if (dir == NULL) {
        return BOOK_INVALID_ARG;
    }

    if (*dir == '\0') {
        return BOOK_NO_PATH;
    }

    size_t idx = strlen(dir) - 1;
    char last = dir[idx];
    if (is_sep(last)) {
        idx--;
        dir[idx] = '\0';
    }

    while (idx > 0 && ! is_sep(dir[idx])) {
        idx--;
    }

    if (idx == 0) {
        return BOOK_NO_PATH;
    }

    dir[idx] = '\0';

    return BOOK_SUCCESS;
}

static int is_dir_inside(char *dir, char *main_dir) {
    if (dir == NULL || main_dir == NULL) {
        return BOOK_INVALID_ARG;
    }

    if (! path_has_root(dir) || ! path_has_root(main_dir)) {
        return BOOK_NO_PATH;
    }

    size_t idx = 0;
    size_t dir_sz = strlen(dir), main_sz = strlen(main_dir);

    if (dir_sz == 0 && main_sz == 0) {
        return BOOK_SUCCESS;
    }
    if (dir_sz == 0 || main_sz == 0) {
        return BOOK_NO_PATH;
    }

    if (dir_sz + 1 == main_sz && main_dir[main_sz - 1] == '/') {
        main_sz--;
    }
    if (dir_sz < main_sz) {
        return BOOK_NO_PATH;
    }

    utf8proc_uint8_t *dir_tmp = (utf8proc_uint8_t *)dir,
                     *main_tmp = (utf8proc_uint8_t *)main_dir;
    while (idx < main_sz) {
        utf8proc_int32_t dchar, mchar;
        size_t d = utf8proc_iterate(dir_tmp, -1, &dchar);
        size_t m = utf8proc_iterate(main_tmp, -1, &mchar);

        if (m != d) {
            return BOOK_NO_PATH;
        }
#ifdef _WIN32
        dchar = utf8proc_tolower(dchar);
        mchar = utf8proc_tolower(mchar);
        int is_both_sep = (d == 1 && is_sep((char)dchar) && is_sep((char)mchar));
        if (! is_both_sep && dchar != mchar) {
            return BOOK_NO_PATH;
        }
#else
        if (dchar != mchar) {
            return BOOK_NO_PATH;
        }
#endif

        idx += d;
        dir_tmp += d;
        main_tmp += m;

        if (! *dir_tmp && (mchar == '\0' || is_sep((char)mchar))) {
            return BOOK_SUCCESS;
        }
    }

    return BOOK_SUCCESS;
}

static int process_utf(char *str, size_t sz, utf_func fn) {
    if (str == NULL) {
        return BOOK_INVALID_ARG;
    }
    if (*str == '\0') {
        return BOOK_SUCCESS;
    }

    if (sz == 0) {
        sz = strlen(str) + 1;
    }

    utf8proc_uint8_t *in = (utf8proc_uint8_t *)str;
    utf8proc_uint8_t *out = malloc(sizeof(utf8proc_uint8_t) * sz * 2);
    if (out == NULL) {
        return BOOK_NO_MEMORY;
    }

    int res = BOOK_SUCCESS;

    utf8proc_uint8_t *out_tmp = out;
    while (*in != '\0') {
        utf8proc_int32_t ichar, ochar;
        size_t d = utf8proc_iterate(in, -1, &ichar);
        if (ichar == -1) {
            res = BOOK_INVALID_UTF;
            break;
        }
        in += d;

        ochar = (*fn)(ichar);
        size_t shift = utf8proc_encode_char(ochar, out_tmp);
        out_tmp += shift;
    }

    if (res == BOOK_SUCCESS) {
        *out_tmp = '\0';
    }

    if (strlen((char *)out) + 1 > sz) {
        res = BOOK_BUFFER_SMALL;
    } else {
        strcpy((char *)str, (char *)out);
    }

    free(out);
    return res;
}

int DLL_EXPORT to_lower(char *str, size_t sz) {
    return process_utf(str, sz, utf8proc_tolower);
}

int DLL_EXPORT to_upper(char *str, size_t sz) {
    return process_utf(str, sz, utf8proc_toupper);
}

int DLL_EXPORT make_relative(char *dir, char *main_dir) {
    int r = is_dir_inside(dir, main_dir);

    if (r != BOOK_SUCCESS) {
        return r;
    }

    size_t msize = strlen(main_dir);
    char *rel_dir = dir + msize;

    if (is_sep(*rel_dir)) {
        rel_dir++;
    }

    if (! *rel_dir) {
        dir[0] = '.';
        dir[1] = '\0';
        return BOOK_SUCCESS;
    }

    char *dest = dir;
    while (*rel_dir) {
        *dest++ = *rel_dir++;
    }
    *dest = '\0';

    return BOOK_SUCCESS;
}

//#ifdef _WIN32
//    // Windows (x64 and x86)
//#elif __unix__ // all unices, not all compilers
//    // Unix
//#elif __linux__
//    // linux
//#elif __APPLE__
//    // Mac OS, not sure if this is covered by __posix__ and/or __unix__ though...
//#endif
