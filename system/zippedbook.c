#include <stdio.h>

#include "utf8proc.h"
#include "bookutil.h"
#include "bookconfig.h"
#include "zippedbook.h"

#ifdef _WIN32
    #include <windows.h>
    #define POPEN wpopen
    #define CHAR wchar_t
#else
    #define POPEN popen
    #define CHAR char
#endif

static int exec_and_get_stdout(char *cmd, char *buf, size_t max_sz) {
    CHAR *cmdline;
#ifdef _WIN32
    CHAR *ATTR = L"r";
    size_t out_sz = sizeof(CHAR) * (strlen(cmd) + 1);
    cmdline = (CHAR *)malloc(out_sz);
    if (cmdline == NULL) {
        return BOOK_NO_MEMORY;
    }
    utf_to_ucs(cmd, cmdline, out_sz);
#else
    cmdline = cmd;
    CHAR *ATTR = "r";
#endif
    int res = BOOK_SUCCESS;

    FILE *fp;
    if ((fp = POPEN(cmdline, ATTR)) != NULL) {
        fgets(buf, max_sz, fp);
    } else {
        res = BOOK_EXEC_FAIL;
    }
    pclose(fp);

#ifdef _WIN32
    free(cmdline);
#endif

    return res;
}

int is_archive(char *arcname, char *filename, size_t buf_sz) {
    if (arcname == NULL || *arcname == '\0') {
        return BOOK_INVALID_FILE;
    }

    const struct reader_conf *conf = get_reader_options();
    if (conf->unzip == NULL || *conf->unzip == '\0') {
        return BOOK_NO_PATH;
    }

    char cmd[2048] = {0};
    strcpy(cmd, "\"");
    strncat(cmd, conf->unzip, 2048);
    strncat(cmd, "\" l -sccUTF-8 \"", 2048);
    strncat(cmd, arcname, 2048);
    strncat(cmd, "\"", 2048);

    char output[4096] = {0};
    int res = exec_and_get_stdout(cmd, output, 4096);

    if (res != BOOK_SUCCESS) {
        return res;
    }

    return BOOK_SUCCESS;
}
