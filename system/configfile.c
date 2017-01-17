#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iconv.h>

#include "utf8proc.h"

#include "configfile.h"

#define MAX_KEY_NAME 256
#define MAX_KEY_VALUE 2048

int load_config(char *path, config_callback cb) {
    if (path == NULL || *path == '\0') {
        return CONFIG_NO_FILE;
    }

    iconv_t h = iconv_open("UTF-16LE", "UTF-8");

    char *wpath = (char *) calloc(sizeof(char), strlen(path) * 8);
    char *inpath = path;
    wchar_t *out_path = (wchar_t*)wpath;
    size_t in_left=strlen(path),
        out_left=strlen(path) * 8;

    iconv(h, &inpath, &in_left, &wpath, &out_left);
    const wchar_t *mode = L"rb";

    FILE *fd = _wfopen(out_path, mode);
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

    fread(cfile, sizeof(char), fsize, fd);
    fclose(fd);
    cfile[fsize] = '\0';
    char *ptr = cfile;
    if (fsize > 2 && fsize[0] == 0xEF && fsize[1] == 0xBB && fsize[2] == 0xBF) {
        ptr += 3;
    }

    char key[MAX_KEY_NAME];
    char val[MAX_KEY_VALUE];
    while (*ptr != '\0') {
        if (*ptr == '\0' || *ptr == 0x0A || *ptr == 0x0D) {
            ptr++;
            continue;
        }

        if (*ptr == '#' || *ptr == ';') {
            // comment - skip till the end of line
            while (*ptr != '\0' || *ptr != 0x0A || *ptr != 0x0D) {
                *ptr++;
            }
        }

        size_t clen;
        utf8proc_uint8_t *src = (utf8proc_uint8_t*)ptr;
        utf8proc_uint8_t *dst = (utf8proc_uint8_t*)key;
        utf8proc_int32_t cp;

        while (*src != '\0') {
            clen = utf8proc_iterate(src, -1, &cp);
            utf8proc_category_t ctg = utf8proc_category(cp);

            if (cp == 0x09 ||
        }
    }

    free(cfile);
    free(wpath);
    return CONFIG_SUCCESS;
}
