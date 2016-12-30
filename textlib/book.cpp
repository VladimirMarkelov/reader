#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>

#include "book.h"
#include "bookutil.h"
#include "bookext.h"
#include "plaintext.h"

struct plugin_info {
    struct plugin_info *next;
    fn_load_book load;
    fn_enc enc;
    fn_supported supported;
    fn_unload_book unload;
    /* TODO: HANDLE to library to keep it loaded */
};
static struct plugin_info *head = NULL;

void load_plugins() {
    /* TODO: look through folder 'plugins' and load all libraries
     * now it is just a stub for plain text. In real life plain text
     * processor must be the last in the list
     */

    head = (struct plugin_info*)malloc(sizeof(plugin_info));
    head->next = NULL;
    head->enc = get_encoding;
    head->load = prepare_book;
    head->supported = can_open;
    head->unload = free_book;
}

void unload_plugins() {
    struct plugin_info *tmp = head;
    while (tmp != NULL) {
        head = tmp;
        tmp = head->next;
        /* TODO: unload library */
        free(head);
    }
}


int book_open(char* path, struct book_info_t *book, int reading) {
    if (book == NULL) {
        return BOOK_LOAD_FAILED;
    }

    book->author = NULL;
    book->text = NULL;
    book->title = NULL;
    book->fd = NULL;
    book->status = BOOK_LOAD_FAILED;
    book->zipped = BOOK_PLAIN_TEXT;
    book->text_sz = 0;
    book->encoding[0] = '\0';

    iconv_t h = iconv_open("UTF-16LE", "UTF-8");

    char *wpath = (char *) calloc(sizeof(char), strlen(path) * 8);
    char *inpath = path;
    wchar_t *out_path = (wchar_t*)wpath;
    size_t in_left=strlen(path),
        out_left=strlen(path) * 8;

    iconv(h, &inpath, &in_left, &wpath, &out_left);

    const wchar_t *mode = (reading == BOOK_READ)? L"rb" : L"wb";
    book->fd = _wfopen(out_path, mode);

    iconv_close(h);
    free(out_path);

    book->status = BOOK_LOAD_SUCCEES;

    /* loop through all plugins and the first one replied that it can parse
     * the book gets book to make it readable
     */
    /* TODO: for the demo, there is no plugins, just default plain text */
    if (head == NULL) {
        load_plugins();
    }

    struct plugin_info* plugin = NULL;
    struct plugin_info* tmp = head;
    while (tmp != NULL) {
        if (tmp->supported(book) == BOOK_SUCCESS) {
            plugin = tmp;
            break;
        }

        tmp = tmp->next;
    }

    if (plugin == NULL) {
        printf("No plugin detected\n");
        book->status = BOOK_PARSE_INVALID_FORMAT;
    } else {
        printf("PLUGIN DETECTED\n");
        if (plugin->enc(book) != BOOK_SUCCESS) {
            printf("Failed to detect encoding\n");
        }
        printf("   enc == %s\n", book->encoding);
        plugin->load(book);
    }

    return book->status;
}

void book_close(book_info_t *book) {
    if (book == NULL) {
        return;
    }

    if (book->fd) {
        fclose(book->fd);
    }
    /*
    free(book->author);
    free(book->title);
    free(book->text);
    */
}

