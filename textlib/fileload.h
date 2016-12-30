#ifndef FILELOAD_H_INCLUDED
#define FILELOAD_H_INCLUDED

struct book_info_t {
    char *author;
    char *title;
    char *text;
};

FILE* file_open(char* path, int reading);
struct book_info_t load_book(const char* path);
void free_book(struct book_info_t *book);

#endif // FILELOAD_H_INCLUDED
