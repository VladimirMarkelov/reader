#include <stdio.h>

#include "logger.h"

#include "book.h"
#include "bookutil.h"
#include "bookext.h"
#include "files.h"
#include "bookconfig.h"

#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    printf("STep 1\n");
    char path[] = "пример.txt";
    printf("STep 2\n");
    struct book_info book_info;
    book_info.encoding[0] = '\0';


    struct reader_conf rconf;
    load_reader_config(&rconf);

    for (int i = 1; i < argc; ++i) {
#ifdef _WIN32
        char utf[512];
        int cnv = ucs_to_utf(argv[i], utf, 512);
        if (cnv == BOOK_SUCCESS) {
            process_app_arg(utf, &rconf);
        }
#else
        process_app_arg(argv[i], &rconf);
#endif
    }
    if (rconf.enc[0] != '\0') {
        log_message("main.log", "Use %s encoding to open book\n", rconf.enc);
        strcpy(book_info.encoding, rconf.enc);
    }

    int res = book_open(path, &book_info, BOOK_READ);
    printf("Open book: %d\n", res);

    char dir[1024];
    char dir2[1024];
    int r = get_user_directory(dir, 1024);
    printf("%d : %s\n", r, dir);
    r = get_app_directory(dir2, 1024);
    printf("%d : %s\n", r, dir2);

    FILE *f = fopen("cnv.txt", "wb");
    fwrite(book_info.text, sizeof(char), book_info.text_sz, f);
    fclose(f);

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

    char cwd[1024];
    int rr = get_working_directory(cwd, 1024);
    log_message("main.log", "[%d] %s\n", rr, cwd);

    return 0;
}
