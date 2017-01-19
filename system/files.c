#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>

#include "logger.h"

#include "bookutil.h"
#include "files.h"

#ifdef _WIN32
    #include <direct.h>
    #define getcwd _wgetcwd // stupid MSFT "deprecation" warning
#else
    #include <unistd.h>
#endif

int get_app_directory(char *dir, size_t len) {
#ifdef _WIN32
    if (dir == NULL) {
        return BOOK_INVALID_ARG;
    }

    #define MAXPATH 1024
    wchar_t path[MAXPATH];
    errno = 0;
    DWORD res = GetModuleFileNameW(NULL, path, MAXPATH);
    if(errno != ERROR_SUCCESS) {
        return BOOK_FAIL;
    }
    if (res > 0) {
        path[res] = L'\0';
    }

    ucs_to_utf(path, dir, len);

    size_t sz = strlen(dir);
    char *end = dir + sz;
    while (end != dir) {
        if (*end == '\\' || *end == '/') {
            *end = '\0';
            break;
        }
        *end = '\0';
        --end;
    }
#else
    char tmp[32];
    sprintf(tmp, "/proc/%d/exe", getpid());
    int bytes = MIN(readlink(tmp, dir, len), len - 1);
    if(bytes >= 0) {
        dir[bytes] = '\0';
    }

    size_t sz = strlen(dir);
    char *end = dir + sz;
    while (end != dir && *end != '/') {
        *end = '\0';
        --end;
    }
#endif
    return BOOK_SUCCESS;
}

int get_working_directory(char *dir, size_t len) {
    if (dir == NULL) {
        return BOOK_INVALID_ARG;
    }

    char *cwd;

#ifdef _WIN32
    iconv_t h = iconv_open("UTF-8", "UTF-16LE");

    size_t outsize = len;
    char *outbuf = dir;
    wchar_t *path = (wchar_t*)malloc(len);
    char *inbuf = (char *)path;

    wchar_t *c = getcwd(path, outsize);

    if (c != NULL) {
        size_t insize = (wcslen(path) + 1) * sizeof(wchar_t);
        errno = 0;
        size_t ires = iconv(h, &inbuf, &insize, &outbuf, &outsize);
        if (ires == (size_t)-1) {
            cwd = NULL;
        } else {
            cwd = dir;
        }
    } else {
        cwd = NULL;
    }

    iconv_close(h);
    free(path);
#else
    cwd = getcwd(dir, len);
#endif

    return cwd == NULL ? BOOK_BUFFER_SMALL : BOOK_SUCCESS;
}
