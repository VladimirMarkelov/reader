#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iconv.h>

#include "utf8proc.h"

#include "bookutil.h"
#include "configfile.h"

#define MAX_KEY_NAME 256
#define MAX_KEY_VALUE 2048

int load_config(char *path, config_callback cb) {
    if (path == NULL || *path == '\0') {
        return CONFIG_NO_FILE;
    }

    iconv_t h = iconv_open("UTF-16LE", "UTF-8");

    char *out_path = (char *) calloc(sizeof(char), strlen(path) * 8);
    char *in_path = path;
    wchar_t *wpath = (wchar_t*)out_path;
    size_t in_left=strlen(path),
        out_left=strlen(path) * 8;

    iconv(h, &in_path, &in_left, &out_path, &out_left);
    const wchar_t *mode = L"rb";

    FILE *fd = _wfopen(wpath, mode);
    iconv_close(h);

    if (fd == NULL) {
        free(wpath);
        return CONFIG_NO_FILE;
    }

    fseek(fd, 0L, SEEK_END);
    size_t fsize = ftell(fd);

    if (fsize > 128 * 1024) {
        fprintf(stderr, "Config file is too big\n");
        fclose(fd);
        free(wpath);
        return CONFIG_INVALID;
    }

    rewind(fd);
    char *cfile = (char *)malloc(sizeof(char) * (fsize + 1));
    if (cfile == NULL) {
        fprintf(stderr, "Out of memory\n");
        fclose(fd);
        free(wpath);
        return CONFIG_NO_MEMORY;
    }

    printf("File size: %d\n", (int)fsize);
    fread(cfile, sizeof(char), fsize, fd);
    fclose(fd);
    cfile[fsize] = '\0';
    char *ptr = cfile;
    if (fsize > 2 &&
        (unsigned char)cfile[0] == 0xEF &&
        (unsigned char)cfile[1] == 0xBB &&
        (unsigned char)cfile[2] == 0xBF) {
        printf("Skipping BOM\n");
        ptr += 3;
    }

    char key[MAX_KEY_NAME];
    char val[MAX_KEY_VALUE];
    int get_val = 0;

    *key = *val = '\0';

    while (*ptr != '\0') {
        if (*ptr == '\0' || *ptr == 0x0A || *ptr == 0x0D) {
            ptr++;
            continue;
        }

        if (*ptr == '#' || *ptr == ';') {
            // comment - skip till the end of line
            while (*ptr != '\0' && *ptr != 0x0A && *ptr != 0x0D) {
                ptr++;
            }
            continue;
        }

        size_t clen;
        utf8proc_uint8_t *src = (utf8proc_uint8_t*)(utf_skip_spaces(ptr, NULL));
        utf8proc_uint8_t *dst = (utf8proc_uint8_t*)key;
        utf8proc_int32_t cp;
        size_t sz = 0, maxsz = MAX_KEY_NAME;

        while (*src != '\0') {
            clen = utf8proc_iterate(src, -1, &cp);

            if (cp == 0x0A || cp == 0x0D) {
                if (get_val == 1) {
                    cb(key, val);
                }
                key[0] = '\0';
                val[0] = '\0';
                sz = 0;
                maxsz = MAX_KEY_NAME;
                dst = (utf8proc_uint8_t*)key;
                get_val = 0;
                src += clen;
                break;
            }

            if ((cp == 0x09 || cp == 0x20) && get_val == 0) {
                get_val = 1;
                dst = (utf8proc_uint8_t*)val;
                src += clen;
                src = (utf8proc_uint8_t*)utf_skip_spaces((char*)src, NULL);
                maxsz = MAX_KEY_VALUE;
                sz = 0;
                continue;
            }

            if (clen + sz < maxsz) {
                utf8proc_encode_char(cp, dst);
                dst += clen;
                *dst = '\0';
            } else {
                sz = maxsz;
            }
            src += clen;
        }

        ptr = (char *)src;
    }
    if (get_val == 1) {
        cb(key, val);
    }

    free(cfile);
    free(wpath);
    return CONFIG_SUCCESS;
}
