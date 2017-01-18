#include <stdio.h>

#include "book.h"
#include "bookutil.h"
#include "bookext.h"
#include "files.h"
#include "bookconfig.h"

int main()
{
    printf("STep 1\n");
    char path[] = "пример.txt";
    printf("STep 2\n");
    struct book_info book_info;
    int res = book_open(path, &book_info, BOOK_READ);
    printf("Open book: %d\n", res);

    //std::string s;
    //if (book_info.text != NULL) {
    //    s = std::string(book_info.text, book_info.text + 15);
    //}

    //cout << res << " --> " << book_info.status << " : " << s << endl;

    char dir[1024];
    char dir2[1024];
    int r = get_user_directory(dir, 1024);
    printf("%d : %s\n", r, dir);
    r = get_app_directory(dir2, 1024);
    printf("%d : %s\n", r, dir2);

    FILE *f = fopen("cnv.txt", "wb");
    fwrite(book_info.text, sizeof(char), book_info.text_sz, f);
    fclose(f);

    struct reader_conf rconf;
    load_reader_config(&rconf);

    struct pre_options opts;
    opts.width = 80;
    opts.hyph_disable = ! rconf.hyphen;
    opts.add_spaces = rconf.widen;

    struct book_preformat* fmt = book_preformat_mono(&book_info, &opts);
    struct book_preformat* crr = fmt;
    if (crr == NULL) {
        printf("Failed to reformat book\n");
    }
    FILE *fr = fopen("cnvfmt.txt", "wb");
    int i = 0;
    while (crr) {
        fwrite(crr->line, sizeof(char), strlen(crr->line), fr);
        putc(0x0A, fr);
        crr = crr->next;
        i++;
    }
    fclose(fr);
    printf("Lines = %d\n", i);

    return 0;
}
