#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <iconv.h>
#include "fileload.h"

struct book_info_t load_book(const char* path) {
    struct book_info_t info;
    info.author = NULL;
    info.title = NULL;
    info.text = NULL;

    return info;
}

void free_book(struct book_info_t *book) {
    if (book) {
        free(book->author);
        free(book->title);
        free(book->text);
    }
    free(book);
}

FILE* file_open(char* path, int reading) {
printf("Step 1\n");
    iconv_t h = iconv_open("UTF-16LE", "UTF-8");
    //iconv_t h = iconv_open(/*"UTF-8"*/"CP1251", "UTF-8");
printf("Step 1.5 - %p\n", h);

    char *wpath = (char *) calloc(sizeof(char), strlen(path) * 8);
    char *inpath = path;
    wchar_t *out_path = (wchar_t*)wpath;
    size_t in_left=strlen(path),
        out_left=strlen(path) * 8;

    size_t res = iconv(h, &inpath, &in_left, &wpath, &out_left);
printf("Step 2 - %d - %d - %d - %d\n", int(res), errno, (int)in_left, (int)out_left);

printf("Step 2.1: %s\n", path);
printf("Step 2.2: %s - %d\n", (char*)out_path, (int)wcslen(out_path));
wprintf(L"WCHAR: %s\n", out_path);
    const wchar_t *mode = reading ? L"rb" : L"wb";
    FILE* file = _wfopen(out_path, mode);
printf("Step 3 - %p - %d\n", file, errno);

    iconv_close(h);
    printf("Step 3.5\n");
    free(out_path);

printf("Step 4\n");
    return file;
}
