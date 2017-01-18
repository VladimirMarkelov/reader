#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>

#include "bookutil.h"
#include "utf8proc.h"

typedef utf8proc_int32_t (*utf_func) (utf8proc_int32_t);

static int is_sep(char c) {
#ifdef _WIN32
    return c == DIR_SEP || c == '/';
#else
    return c == DIR_SEP;
#endif
}

int path_is_absolute(char *dir) {
    if (dir == NULL || *dir == '\0') {
        return 0;
    }

#ifdef _WIN32
    return ((dir[0] >= 'A' && dir[0] <= 'Z') || (dir[0] >= 'a' && dir[0] <= 'z'))
           && dir[1] == ':'
           && (dir[2] == '\0' || is_sep(dir[2]));
#else
    return dir[0] == '/';
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
        {1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1 ,1,0,0,0,1, 1,1},
        {1,1,1,1,1, 1,1,0,1,0, 1,1,1,1,1, 0,1,1,1,1, 0,1,1,1,1, 1,1,1,1,0, 1,1},
        {1,1,1,1,1, 1,0,1,1,0, 1,1,1,1,1, 1,1,1,1,1, 0,1,1,1,1, 1,1,1,1,0, 0,1},
        {1,1,1,1,1, 1,0,0,1,0, 1,1,1,1,1, 0,1,1,1,1, 0,0,0,1,1, 0,0,0,0,0, 0,0},
        {1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 0,1,1,1,1, 0,1,1,1,0, 1,1}, //5
        {1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,0,0,1,1, 1,1},
        {1,1,0,1,1, 1,1,0,1,0, 1,1,1,1,1, 1,1,1,0,1, 0,0,0,1,0, 0,0,0,1,0, 0,0},
        {1,1,1,1,1, 1,1,1,1,0, 1,1,1,1,1, 1,1,1,1,1, 0,0,0,1,1, 0,1,1,1,1, 1,1},
        {1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,0,0,0,1, 1,1},
        {0,0,0,1,1, 1,0,1,0,0, 1,1,1,1,1, 1,0,1,1,1, 1,1,1,1,1, 0,0,0,0,0, 0,1}, //10
        {1,0,1,0,0, 1,1,1,1,0, 1,1,0,1,1, 0,1,1,1,1, 0,0,1,0,1, 0,0,0,0,0, 1,0},
        {1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,0,1,1,1, 0,1,0,1,1, 1,0,1,1,0, 1,1},
        {1,1,1,1,1, 1,0,0,1,0, 1,1,1,1,1, 1,1,1,0,1, 0,1,1,1,1, 0,0,1,1,1, 0,1},
        {1,1,1,1,1, 1,1,1,1,0, 1,0,1,1,1, 0,1,1,1,1, 1,0,1,1,1, 1,0,1,1,1, 1,1},
        {1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,0,0,0,1, 1,1}, //15
        {1,0,0,0,0, 1,0,0,1,0, 1,1,0,1,1, 1,1,1,1,1, 0,0,1,1,1, 0,0,1,1,1, 1,1},
        {1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,0,1,1,0, 1,1},
        {1,1,1,1,1, 1,1,1,1,0, 1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,1},
        {1,1,1,1,1, 1,0,1,1,0, 1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,1},
        {1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,1, 1,1,1,1,0, 0,1,1,1,1, 1,0,0,0,1, 1,1}, //20
        {1,0,0,0,0, 1,0,0,1,0, 0,1,1,1,1, 0,1,1,1,1, 0,0,0,0,0, 0,0,1,0,0, 1,0},
        {1,0,1,1,1, 1,0,0,1,0, 1,1,1,1,1, 1,1,1,1,1, 0,0,0,0,1, 0,0,0,0,1, 0,0},
        {1,0,1,0,0, 1,0,0,1,0, 1,1,1,0,1, 0,0,0,0,1, 0,0,0,0,0, 0,0,1,0,0, 0,0},
        {1,0,1,0,0, 1,0,0,1,0, 1,1,1,1,1, 0,1,0,1,1, 1,0,0,0,1, 0,0,0,1,0, 0,0},
        {1,0,1,0,1, 1,0,0,1,0, 1,1,1,1,1, 1,1,1,1,1, 0,1,1,1,0, 0,0,0,1,0, 1,0}, //25
        {1,0,0,0,0, 1,0,0,1,0, 0,0,0,1,1, 0,1,0,0,1, 0,0,0,0,0, 0,0,0,1,0, 0,0},
        {0,0,0,0,0, 1,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 1,1},
        {0,1,1,1,1, 1,1,1,1,1, 1,1,1,1,0, 1,1,1,1,1, 0,1,1,1,1, 1,0,0,0,0, 0,1},
        {0,1,1,1,1, 1,0,1,1,0, 1,0,1,1,1, 0,0,1,1,0, 1,1,1,1,1, 1,0,0,1,0, 1,1},
        {0,0,0,1,0, 1,0,1,0,0, 0,1,1,1,0, 1,1,1,1,1, 1,1,0,0,0, 0,0,0,0,0, 0,0}, //30
        {0,1,1,1,1, 1,1,1,0,1, 1,1,1,1,0, 1,1,1,1,0, 0,1,1,1,1, 1,0,0,0,0, 1,0},
        {0,1,1,1,1, 1,1,1,1,1, 1,1,1,1,0, 1,1,1,1,1, 0,1,1,1,1, 1,0,0,0,0, 1,1}
    };

    size_t non_ascii_cnt = 0;
    size_t real_size = 0;
    size_t cnts[3] = {0, 0, 0};
    size_t ncnts[3] = {0, 0, 0};
    int *maps[3] = {win, alt, koi};

    for (int en = 0; en < 3; en++) {
        size_t idx = 0;
        unsigned char *str = (unsigned char *)text;
        unsigned char prev = 128;
        real_size = 0;

        while (idx < sz && *str != '\0') {
            real_size++;

            if (*str < 128) {
                prev = 128;
            } else {
                non_ascii_cnt++;
                size_t isgood = maps[en][*str - 128];

                if (isgood > 0) {
                    if (prev < 128) {
                        if (chardouble[prev-1][isgood-1]) {
                            cnts[en]++;
                        } else {
                            ncnts[en]++;
                        }
                    }
                    prev = isgood;
                } else {
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

    size_t minsz = real_size / 10;
    if (cnts[0] > cnts[1] && cnts[0] > cnts[2] && cnts[0] > minsz) {
        return ENC_WIN1251;
    } else if (cnts[1] > cnts[0] && cnts[1] > cnts[2] && cnts[1] > minsz) {
        return ENC_DOS866;
    } else if (cnts[2] > cnts[0] && cnts[2] > cnts[1] && cnts[2] > minsz) {
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

int DLL_EXPORT is_encoding_supported(const char *enc) {
    static const char* encodings[] = {
        /* European */
        "ASCII", "ISO-8859-1", "ISO-8859-2", "ISO-8859-3", "ISO-8859-4",
        "ISO-8859-5", "ISO-8859-7", "ISO-8859-9", "ISO-8859-10", "ISO-8859-13",
        "ISO-8859-14", "ISO-8859-15", "ISO-8859-16", "KOI8-R", "KOI8-U",
        "KOI8-RU", "CP1250", "CP1251", "CP1252", "CP1253", "CP1254", "CP1257",
        "CP850", "CP866", "CP1131", "MacRoman", "MacCentralEurope",
        "MacIceland", "MacCroatian", "MacRomania", "MacCyrillic", "MacUkraine",
        "MacGreek", "MacTurkish", "Macintosh",
        /* Hebrew */
        "ISO-8859-6", "ISO-8859-8", "CP1255", "CP1256", "CP862", "MacHebrew",
        "MacArabic",
        /* Japanese */
        "EUC-JP", "SHIFT_JIS", "CP932", "ISO-2022-JP", "ISO-2022-JP-2",
        "ISO-2022-JP-1",
        /* Chinese */
        "EUC-CN", "HZ", "GBK", "CP936", "GB18030", "EUC-TW", "BIG5", "CP950",
        "BIG5-HKSCS", "BIG5-HKSCS:2004", "BIG5-HKSCS:2001", "BIG5-HKSCS:1999",
        "ISO-2022-CN", "ISO-2022-CN-EXT",
        /* Korean */
        "EUC-KR", "CP949", "ISO-2022-KR", "JOHAB",
        /* Full Unicode */
        "UTF-8", "UCS-2", "UCS-2BE", "UCS-2LE", "UCS-4", "UCS-4BE", "UCS-4LE",
        "UTF-16", "UTF-16BE", "UTF-16LE", "UTF-32", "UTF-32BE", "UTF-32LE",
        "UTF-7", "C99",
        /* Last one */
        NULL
    };

    if (enc == NULL || *enc == '\0') {
        return 0;
    }

    size_t idx = 0;
    while (encodings[idx]) {
        if (strcmp(encodings[idx], enc) == 0) {
            return 1;
        }
        idx++;
    }

    return 0;
}

size_t convert_to_utf8_buffer(char *in, size_t in_sz, char *out, size_t out_sz, const char *enc) {
    if (in == NULL || enc == NULL) {
        return (size_t)-1;
    }

    if (strncmp("UTF-8", enc, 5) == 0 || strncmp("ASCII", enc, 5) == 0 ) {
        return 0;
    }

    if (! is_encoding_supported(enc)) {
        return (size_t)-3;
    }

    if (in_sz == 0) {
        in_sz = strlen(in) + 1;
    }

    if (out == NULL) {
        return 4 * in_sz;
    }

    size_t in_size = in_sz;
    size_t max_size = out_sz;

    iconv_t h = iconv_open("UTF-8", enc);
    char *inbuf = in, *outbuf = out;
    size_t res = iconv(h, &inbuf, &in_size, &outbuf, &max_size);
    iconv_close(h);

    if (res == (size_t)-1) {
        return (size_t)-2;
    }

    return out_sz - max_size;
}

size_t convert_from_utf8_buffer(char *in, size_t in_sz, char *out, size_t out_sz, const char *enc) {
    if (in == NULL || enc == NULL) {
        return (size_t)-1;
    }

    if (strncmp("UTF-8", enc, 5) == 0 || strncmp("ASCII", enc, 5) == 0 ) {
        return 0;
    }

    if (! is_encoding_supported(enc)) {
        return (size_t)-3;
    }

    if (in_sz == 0) {
        in_sz = strlen(in) + 1;
    }

    if (out == NULL) {
        return 2 * in_sz;
    }

    size_t in_size = in_sz;
    size_t max_size = out_sz;

    iconv_t h = iconv_open(enc, "UTF-8");
    char *inbuf = in, *outbuf = out;
    size_t res = iconv(h, &inbuf, &in_size, &outbuf, &max_size);
    iconv_close(h);

    if (res == (size_t)-1) {
        return (size_t)-2;
    }

    return out_sz - max_size;
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
    // TODO:
    return BOOK_FAIL;
#endif

    return BOOK_SUCCESS;
}

int DLL_EXPORT get_temp_directory(char *dir, size_t len) {
    if (dir == NULL) {
        return BOOK_INVALID_ARG;
    }

#ifdef _WIN32
    wchar_t *vars[] = {
        L"TMPDIR", L"TMP", L"TEMP", L"TEMPDIR", L"USERPROFILE", NULL
    };
    size_t idx = 0;
    while (vars[idx] != NULL) {
        const wchar_t *path = _wgetenv(vars[idx]);

        if (path == NULL || *path == '\0') {
            idx++;
            continue;
        }

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

        break;
    }
#else
    char *vars[] = {
        "TMPDIR", "TMP", "TEMP", "TEMPDIR", "USERPROFILE", NULL
    };
    size_t idx = 0;
    while (vars[idx] != NULL) {
        const char *path = getenv(vars[idx]);

        if (path == NULL || *path == '\0') {
            idx++;
            continue;
        }

        if (strlen(path) + 1 > len) {
            return (errno == E2BIG) ? BOOK_BUFFER_SMALL : BOOK_CONVERT_FAIL;
        }

        strcpy(dir, path);
        break;
    }
#endif // _WIN32

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

    if (! path_is_absolute(dir) || ! path_is_absolute(main_dir)) {
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

size_t DLL_EXPORT utf_len(char *str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }

    size_t len = 0;
    utf8proc_uint8_t *tmp = (utf8proc_uint8_t*)str;
    utf8proc_int32_t cp;
    while (*tmp) {
        size_t sz = utf8proc_iterate(tmp, -1, &cp);
        tmp += sz;
        len++;
    }

    return len;
}

char* DLL_EXPORT utf_advance(char *str, size_t cnt) {
    if (str == NULL || *str == '\0' || cnt == 0) {
        return str;
    }

    size_t passed = 0;
    utf8proc_uint8_t *tmp = (utf8proc_uint8_t*)str;
    utf8proc_int32_t cp;
    while (passed < cnt && *tmp != 0) {
        size_t sz = utf8proc_iterate(tmp, -1, &cp);
        tmp += sz;
        passed++;
    }

    return (char*)tmp;
}

int DLL_EXPORT utf_substr(char *str, size_t from, size_t to, char *dest, size_t dest_sz) {
    if (str == NULL || dest == NULL || dest_sz == 0) {
        return BOOK_INVALID_ARG;
    }

    char *s = utf_advance(str, from);
    if (*s == '\0') {
        *dest = '\0';
        return BOOK_SUCCESS;
    }

    utf8proc_uint8_t *src_tmp = (utf8proc_uint8_t*)s;
    utf8proc_uint8_t *dest_tmp = (utf8proc_uint8_t*)dest;
    utf8proc_int32_t cp;
    while (*src_tmp != '\0' && from < to) {
        size_t sz = utf8proc_iterate(src_tmp, -1, &cp);
        src_tmp += sz;
        from++;

        if (dest_sz != (size_t)-1) {
            if (dest_sz <= sz) {
                return BOOK_BUFFER_SMALL;
            }

            dest_sz -= sz;
        }

        utf8proc_encode_char(cp, dest_tmp);
        dest_tmp += sz;
    }

    *dest_tmp = '\0';
    return BOOK_SUCCESS;
}

// hyph - array of substring lengths that can be left on the first line
int DLL_EXPORT hyphenation(char *str, size_t *hyph, size_t sz) {
    char *vowel = "aoeiuyаеёиоуэыюяáóíúéýöüïąįųůøæũãõĩŭ";
    char *cons  = "бвгджзклмнпрстфхцчшщbcdfghjklmnpqrstvwxzčȟšŋǧłþř";
    int res = BOOK_SUCCESS;

    if (sz == 0 || hyph == NULL || str == NULL) {
        return BOOK_INVALID_ARG;
    }

    size_t str_len = utf_len(str);
    if (str_len < 3) {
        hyph[0] = 0;
        return BOOK_SUCCESS;
    }

    char *mapped = (char *)calloc(str_len + 1, sizeof(char));

    size_t idx = 0;
    size_t c_len;
    utf8proc_uint8_t *tmp = (utf8proc_uint8_t*)str;
    utf8proc_int32_t cp;
    while (idx < str_len) {
        c_len = utf8proc_iterate(tmp, -1, &cp);
        cp = utf8proc_tolower(cp);
        tmp += c_len;

        char lookfor[5];
        utf8proc_encode_char(cp, (utf8proc_uint8_t*)lookfor);
        lookfor[c_len] = '\0';
        if (strstr(vowel, lookfor) != NULL) {
            mapped[idx] = 'v';
        } else if (strstr(cons, lookfor) != NULL) {
            mapped[idx] = 'c';
        } else {
            mapped[idx] = 'x';
        }
        idx++;
    }

    // rules
    // x- , v-v, vc-vc, cv-cv, vc-ccv, vcc-ccv
    idx = 0;
    size_t rule = 0;
    while (idx < str_len) {
        char c = mapped[idx];
        int ok = 0;

        if (idx == 0 || idx == str_len - 1) {
            idx++;
            continue;
        }

        if (c == 'x') {
            ok = 1;
        } else if (c == 'v') {
            if (idx < str_len - 1 && mapped[idx + 1] == 'v') {
                ok = 1;
            } else if (idx > 0 && idx < str_len - 2 &&
                       mapped[idx - 1] == 'c' &&
                       mapped[idx + 1] == 'c' &&
                       mapped[idx + 2] == 'v') {
                ok = 1;
            }
        } else if (c == 'c' && idx > 0 && idx < str_len - 2) {
            if (mapped[idx - 1] == 'c') {
                if (idx > 1 && idx < str_len - 3) {
                    if (mapped[idx - 2] == 'v' &&
                        mapped[idx + 1] == 'c' &&
                        mapped[idx + 2] == 'c' &&
                        mapped[idx + 3] == 'v') {
                            ok = 1;
                    }
                }
            } else if (mapped[idx - 1] == 'v') {
                if ((mapped[idx + 1] == 'v' &&
                     mapped[idx + 2] == 'c') ||
                    (mapped[idx + 1] == 'c' &&
                     mapped[idx + 2] == 'v') ||
                    (idx < str_len - 3 &&
                     mapped[idx + 1] == 'c' &&
                     mapped[idx + 2] == 'c' &&
                     mapped[idx + 3] == 'v'))
                {
                    ok = 1;
                }
            }
        }

        idx++;
        if (ok) {
            hyph[rule] = idx;
            mapped[idx - 1] = '-';
            rule++;
            if (rule > sz) {
                res = BOOK_BUFFER_SMALL;
                break;
            }
        }
    }

    if (res == BOOK_SUCCESS) {
        hyph[rule] = 0;
        /*strcpy(str, mapped);*/
    }

    free(mapped);
    return res;
}

int DLL_EXPORT detect_zip(const char *text, size_t sz) {
    if (text == NULL || *text == '\0') {
        return BOOK_UNKNOWN_FORMAT;
    }

    if (sz == 0) {
        sz = strlen(text);
    }

    if (sz < 4) {
        return BOOK_UNKNOWN_FORMAT;
    }

    if (text[0] == 0x50 && text[1] == 0x4b) {
        if (text[2] == 0x3 && text[3] == 0x4) {
            return BOOK_ZIP;
        } else if ((text[2] == 0x5 || text[2] == 0x6) && text[3] == 0x6) {
            return BOOK_EMPTY_ZIP;
        }
    }

    return BOOK_PLAIN_TEXT;
}

static struct ext_buffer_node_t* ext_buffer_add_node(struct ext_buffer_t *buf) {
    if (buf == NULL || (buf->current == NULL && buf->head != NULL)) {
        return NULL;
    }

    struct ext_buffer_node_t* tmp = (struct ext_buffer_node_t*)malloc(sizeof(struct ext_buffer_node_t));
    if (tmp == NULL) {
        return NULL;
    }
    tmp->data = (char *)malloc(sizeof(char) * EXT_BUF_NODE_SIZE);
    if (tmp->data == NULL) {
        free(tmp);
        return NULL;
    }
    tmp->next = NULL;
    tmp->used = 0;

    if (buf->head == NULL) {
        buf->head = tmp;
        buf->current = tmp;
        return tmp;
    }

    buf->current->next = tmp;
    buf->current = tmp;
    return tmp;
}

struct ext_buffer_t* DLL_EXPORT ext_buffer_init() {
    struct ext_buffer_t *b = (struct ext_buffer_t*)malloc(sizeof(struct ext_buffer_t));
    if (! b) {
        return b;
    }
    b->head = b->current = NULL;

    struct ext_buffer_node_t* new_node = ext_buffer_add_node(b);
    if (new_node == NULL) {
        free(b);
        return NULL;
    }

    return b;
}

int DLL_EXPORT ext_buffer_destroy(struct ext_buffer_t *buf) {
    if (buf == NULL || buf->head == NULL) {
        return BOOK_SUCCESS;
    }

    struct ext_buffer_node_t* node = buf->head;
    while (node != NULL) {
        struct ext_buffer_node_t* tmp = node->next;
        free(node->data);
        free(node);
        node = tmp;
    }

    free(buf);
    return BOOK_SUCCESS;
}

size_t DLL_EXPORT ext_buffer_size(const struct ext_buffer_t *buf) {
    if (buf == NULL || buf->head == NULL) {
        return 0;
    }

    size_t total = 0;
    struct ext_buffer_node_t* node = buf->head;
    while (node != NULL) {
        total += node->used;
        node = node->next;
    }

    return total;
}

int DLL_EXPORT ext_buffer_put_char(struct ext_buffer_t *buf, char c) {
    if (buf == NULL || buf->current == NULL) {
        return BOOK_INVALID_ARG;
    }

    if (buf->current->used == EXT_BUF_NODE_SIZE - 1) {
        buf->current->data[EXT_BUF_NODE_SIZE - 1] = '\0';
        struct ext_buffer_node_t* new_node = ext_buffer_add_node(buf);
        if (new_node == NULL) {
            return BOOK_NO_MEMORY;
        }
    }

    buf->current->data[buf->current->used++] = c;
    return BOOK_SUCCESS;
}

int DLL_EXPORT ext_buffer_put_string(struct ext_buffer_t *buf, char *str, size_t sz) {
    if (buf == NULL || buf->current == NULL) {
        return BOOK_INVALID_ARG;
    }

    if (sz == 0) {
        sz = strlen(str);
    }

    if (buf->current->used + sz >= EXT_BUF_NODE_SIZE) {
        buf->current->data[buf->current->used] = '\0';
        struct ext_buffer_node_t* new_node = ext_buffer_add_node(buf);
        if (new_node == NULL) {
            return BOOK_NO_MEMORY;
        }
    }

    char *tmpstr = str;
    size_t ptr = buf->current->used;
    char *buftmp = buf->current->data + ptr;

    for (size_t idx = 0; idx < sz; idx++) {
        *buftmp++ = *tmpstr++;
    }
    buf->current->used += sz;

    return BOOK_SUCCESS;
}

int DLL_EXPORT ext_buffer_copy_data(const struct ext_buffer_t *buf, char *dest, size_t dest_sz) {
    if (buf == NULL || buf->current == NULL || dest == NULL) {
        return BOOK_INVALID_ARG;
    }

    size_t ext_size = ext_buffer_size(buf);
    if (ext_size + 1 > dest_sz) {
        return BOOK_BUFFER_SMALL;
    }

    if (buf->head == NULL || buf->head->used == 0) {
        *dest = '\0';
        return BOOK_SUCCESS;
    }

    struct ext_buffer_node_t* node = buf->head;
    size_t offset = 0;
    while (node != NULL) {
        memcpy(dest + offset, node->data, node->used);
        offset += node->used;
        node = node->next;
    }
    dest[ext_size] = '\0';

    return BOOK_SUCCESS;
}

char DLL_EXPORT ext_buffer_last_char(const struct ext_buffer_t *buf) {
    if (buf == NULL || buf->current == NULL || buf->current->used == 0) {
        return '\0';
    }

    return buf->current->data[buf->current->used - 1];
}

static int utf_cp_is_space(utf8proc_int32_t cp) {
    if (cp == 0x09) {
        return 1;
    }

    utf8proc_category_t ctg = utf8proc_category(cp);
    return ctg == UTF8PROC_CATEGORY_ZS;
}

char* DLL_EXPORT utf_skip_spaces(char *str, size_t *count) {
    if (str == NULL || *str == '\0') {
        return str;
    }

    utf8proc_uint8_t *tmp = (utf8proc_uint8_t*)str;
    utf8proc_int32_t cp;
    size_t c_len;
    if (count != NULL) {
        *count = 0;
    }

    while (*tmp != '\0') {
        c_len = utf8proc_iterate(tmp, -1, &cp);

        if (! utf_cp_is_space(cp)) {
            break;
        }

        tmp += c_len;
        if (count != NULL) {
            (*count)++;
        }
    }

    return (char*)tmp;
}

char* DLL_EXPORT utf_skip_newline(char *str) {
    if (str == NULL || *str == '\0') {
        return str;
    }

    if (*str == 0x0A) {
        str++;
    } else if (*str == 0x0D) {
        str++;
        if (*str == 0x0A) {
            str++;
        }
    }

    return str;
}

char* DLL_EXPORT utf_next_line(char *str) {
    char *line_end = utf_end_of_line(str);

    if (line_end == NULL || *line_end == '\0') {
        return line_end;
    }

    if (*line_end == 0x0A) {
        line_end++;
    } else if (*line_end == 0x0D) {
        line_end++;
        if (*line_end == 0x0A) {
            line_end++;
        }
    }

    return line_end;
}

char* DLL_EXPORT utf_end_of_line(char *str) {
    if (str == NULL || *str == '\0') {
        return str;
    }

    utf8proc_uint8_t *tmp = (utf8proc_uint8_t*)str;
    utf8proc_int32_t cp;
    size_t c_len;

    while (*tmp != '\0') {
        c_len = utf8proc_iterate(tmp, -1, &cp);

        if (cp == 0x0A || cp == 0x0D) {
            break;
        }

        tmp += c_len;
    }

    return (char *) tmp;
}

size_t DLL_EXPORT utf_line_length(char *str, size_t *spaces) {
    size_t len = 0;
    if (spaces != NULL) {
        *spaces = 0;
    }
    if (str == NULL || *str == '\0') {
        return 0;
    }

    char *no_sp = utf_skip_spaces(str, &len);
    if (spaces != NULL) {
        *spaces = len;
    }
    utf8proc_uint8_t *tmp = (utf8proc_uint8_t*)no_sp;
    utf8proc_int32_t cp;
    size_t c_len;
    while (*tmp != '\0' && *tmp != '\x0A' && *tmp != '\x0D') {
        c_len = utf8proc_iterate(tmp, -1, &cp);
        tmp += c_len;
        len++;
    }

    return len;
}

char* DLL_EXPORT utf_word_forward(char *str, char** word_start) {
    if (str == NULL || *str == '\0') {
        if (word_start != NULL && str != NULL) {
            *word_start = str;
        }
        return str;
    }

    char *next_letter = utf_skip_spaces(str, NULL);
    if (word_start != NULL) {
        *word_start = next_letter;
    }

    utf8proc_uint8_t *tmp = (utf8proc_uint8_t*)next_letter;
    utf8proc_int32_t cp;
    size_t c_len;

    while (*tmp != '\0') {
        c_len = utf8proc_iterate(tmp, -1, &cp);
        if (utf_cp_is_space(cp) || cp == 0x0A || cp == 0x0D) {
            break;
        }

        tmp += c_len;
    }

    return (char *)tmp;
}

char* DLL_EXPORT utf_line_forward(char *str, char** word_start) {
    if (str == NULL || *str == '\0') {
        if (word_start != NULL && str != NULL) {
            *word_start = str;
        }
        return str;
    }

    char *next_letter = utf_skip_spaces(str, NULL);
    if (word_start != NULL) {
        *word_start = next_letter;
    }

    utf8proc_uint8_t *tmp = (utf8proc_uint8_t*)next_letter;
    utf8proc_int32_t cp;
    size_t c_len;

    while (*tmp != '\0') {
        c_len = utf8proc_iterate(tmp, -1, &cp);
        if (cp == 0x0A || cp == 0x0D) {
            break;
        }

        tmp += c_len;
    }

    return (char *)tmp;
}

int DLL_EXPORT utf_word_count(char *str, size_t maxch) {
    if (str == NULL || *str == '\0') {
        return 0;
    }

    int cnt = 0;
    int spaces = 1;
    utf8proc_uint8_t *tmp = (utf8proc_uint8_t*)str;
    utf8proc_int32_t cp;
    size_t c_len, total = 0;

    while (*tmp != '\0' && *tmp != 0x0D && *tmp != 0x0A) {
        c_len = utf8proc_iterate(tmp, -1, &cp);
        total += c_len;
        if (spaces && ! utf_cp_is_space(cp)) {
            if (*tmp != '\0' && *tmp != 0x0A && *tmp != 0x0D) {
                cnt++;
            }
            spaces = 0;
        } else if (! spaces && utf_cp_is_space(cp)) {
            spaces = 1;
        }
        tmp += c_len;

        if (maxch > 0 && total >= maxch) {
            break;
        }
    }

    return cnt;
}

int DLL_EXPORT utf_starts_with(char *str, char *pattern) {
    if (str == NULL || pattern == NULL) {
        return BOOK_INVALID_ARG;
    }
    if (*pattern == '\0') {
        return BOOK_EQUAL;
    }

    utf8proc_uint8_t *tmpstr = (utf8proc_uint8_t*)str;
    utf8proc_int32_t cpstr;
    utf8proc_uint8_t *tmppat = (utf8proc_uint8_t*)pattern;
    utf8proc_int32_t cppat;
    size_t lstr, lpat;

    while (*tmpstr != '\0' && *tmppat != '\0') {
        lstr = utf8proc_iterate(tmpstr, -1, &cpstr);
        lpat = utf8proc_iterate(tmppat, -1, &cppat);

        if (lstr != lpat || cpstr != cppat) {
            return BOOK_NOT_EQUAL;
        }

        tmpstr += lstr;
        tmppat += lpat;
    }

    return (*tmppat == '\0') ? BOOK_EQUAL : BOOK_NOT_EQUAL;
}

int DLL_EXPORT detect_book(const char *text, size_t sz) {
    if (text == NULL || *text == '\0' || sz < 4) {
        return BOOK_FAIL;
    }

    return text[0] == 't' && text[1] == 'r' && text[2] == 'f' &&
           text[3] == 0x6f && text[4] == 0x6f;
}

int DLL_EXPORT utf_line_is_empty(char *str) {
    if (str == NULL || *str == '\0') {
        return 1;
    }

    utf8proc_uint8_t *tmp = (utf8proc_uint8_t*)str;
    utf8proc_int32_t cp;
    size_t c_len;
    int is_empty = 1;

    while (*tmp != '\0' && *tmp != 0x0D && *tmp != 0x0A) {
        c_len = utf8proc_iterate(tmp, -1, &cp);
        if (! utf_cp_is_space(cp)) {
            is_empty = 0;
            break;
        }
        tmp += c_len;
    }

    return is_empty;
}

int DLL_EXPORT utf_is_first_char_lower(char *str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }

    utf8proc_uint8_t *tmp = (utf8proc_uint8_t*)str;
    utf8proc_int32_t cp, cp_lower, cp_upper;
    utf8proc_iterate(tmp, -1, &cp);

    if (cp == -1) {
        return 0;
    }

    cp_lower = utf8proc_tolower(cp);
    cp_upper = utf8proc_toupper(cp);

    return (cp_lower != cp_upper) && (cp == cp_lower);
}

int DLL_EXPORT utf_is_first_char_upper(char *str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }

    utf8proc_uint8_t *tmp = (utf8proc_uint8_t*)str;
    utf8proc_int32_t cp, cp_lower, cp_upper;
    utf8proc_iterate(tmp, -1, &cp);

    if (cp == -1) {
        return 0;
    }

    cp_lower = utf8proc_tolower(cp);
    cp_upper = utf8proc_toupper(cp);

    return (cp_lower != cp_upper) && (cp == cp_upper);
}

int DLL_EXPORT utf_make_wide(char *str, size_t buf_sz, size_t width) {
    if (str == NULL || width > 250) {
        return BOOK_INVALID_ARG;
    }

    size_t chw = utf_len(str);
    if (width < chw) {
        return BOOK_CONVERT_FAIL;
    }

    size_t sp_diff = width - chw;
    if (sp_diff > 10) {
        return BOOK_SUCCESS;
    }

    if (buf_sz < (strlen(str) + sp_diff)) {
        return BOOK_BUFFER_SMALL;
    }

    size_t words = utf_word_count(str, 0), curr_wrd = 0;

    if (words < 2) {
        return BOOK_SUCCESS;
    }

    double to_add = (double)(sp_diff) / (double)(words - 1), curr_sp = 0.0;
    char *tmp = (char*)malloc(sizeof(char) * buf_sz);
    if (tmp == NULL) {
        return BOOK_NO_MEMORY;
    }
    tmp[buf_sz - 1] = '\0';

    utf8proc_uint8_t *utftmp = (utf8proc_uint8_t*)tmp;
    utf8proc_uint8_t *utfstr = (utf8proc_uint8_t*)str;
    utf8proc_int32_t cp;
    size_t c_len, real_sz = 0, real_bytes = 0, sp_put = 0, prev_sp = 0;

    while (*utfstr != '\0') {
        c_len = utf8proc_iterate(utfstr, -1, &cp);

        if (sp_put < sp_diff && utf_cp_is_space(cp)) {
            curr_wrd++;
            if (curr_wrd == words - 1) {
                while (sp_put < sp_diff) {
                    *utftmp = ' ';
                    utftmp++;
                    real_bytes += c_len;
                    real_sz++;
                    sp_put++;
                }
                utf8proc_encode_char(cp, utftmp);
                utfstr += c_len;
                utftmp += c_len;
                real_bytes += c_len;
                real_sz++;
            } else {
                curr_sp += to_add;
                int spcnt = (int)curr_sp - prev_sp;
                prev_sp = (int)curr_sp;
                while (spcnt > 0) {
                    *utftmp = ' ';
                    utftmp++;
                    real_bytes += c_len;
                    real_sz++;
                    spcnt--;
                    sp_put++;
                }

                /* copy all whitespaces */
                while (*utfstr != '\0' && utf_cp_is_space(cp)) {
                    utf8proc_encode_char(cp, utftmp);
                    utfstr += c_len;
                    utftmp += c_len;
                    real_bytes += c_len;
                    real_sz++;
                    c_len = utf8proc_iterate(utfstr, -1, &cp);
                }
            }
        } else {
            utf8proc_encode_char(cp, utftmp);
            utfstr += c_len;
            utftmp += c_len;
            real_bytes += c_len;
            real_sz++;
        }
    }

    *utftmp = '\0';
    strcpy(str, tmp);
    free(tmp);

    return BOOK_SUCCESS;
}

int DLL_EXPORT utf_equal_no_case(const char *str1, const char *str2) {
    if (str1 == NULL && str2 == NULL) {
        return 1;
    }
    if (str1 == NULL || str2 == NULL) {
        return 0;
    }

    utf8proc_uint8_t *utfstr1 = (utf8proc_uint8_t*)str1;
    utf8proc_uint8_t *utfstr2 = (utf8proc_uint8_t*)str2;
    utf8proc_int32_t cp1, cp2;
    size_t l1, l2;

    while (1) {
        if (*utfstr1 == '\0' && *utfstr2 == '\0') {
            return 1;
        }
        if (*utfstr1 == '\0' || *utfstr2 == '\0') {
            return 0;
        }

        l1 = utf8proc_iterate(utfstr1, -1, &cp1);
        l2 = utf8proc_iterate(utfstr1, -1, &cp2);
        if (l1 != l2 || cp1 != cp2) {
            return 0;
        }

        if (cp1 == -1 || cp2 == -1) {
            return -1;
        }

        utfstr1 += l1;
        utfstr2 += l2;
    }

    return 1;
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
