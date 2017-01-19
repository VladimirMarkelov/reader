#include <stdlib.h>
#include <stdio.h>

#include "configfile.h"
#include "bookutil.h"
#include "files.h"

#include "logger.h"

#include "bookconfig.h"

static struct reader_conf local_conf;

static void config_cb(const char *key, const char *val) {
    if (key == NULL || val == NULL) {
        return;
    }
    if (*key == '\0' || *val == '\0') {
        return;
    }

    int v;
    if (utf_equal_no_case(key, "EqualWidth") == 1) {
        if (isdigit(*val)) {
            v = atoi(val);
        } else {
            v = (utf_equal_no_case(val, "true") == 1);
        }
        log_message("main.log", "Set option %s to %d [%s]\n", key, (int)v, val);
        local_conf.widen = v;
    } else if (utf_equal_no_case(key, "Hyphenation") == 1) {
        if (isdigit(*val)) {
            v = atoi(val);
        } else {
            v = (utf_equal_no_case(val, "true") == 1);
        }
        log_message("main.log", "Set option %s to %d [%s]\n", key, (int)v, val);
        local_conf.hyphen = v;
    } else if (utf_equal_no_case(key, "7zip") == 1) {
        log_message("main.log", "Set option %s to [%s]\n", key, val);
        if (local_conf.unzip != NULL) {
            free(local_conf.unzip);
        }
        local_conf.unzip = (char*)malloc(sizeof(char) * (strlen(val) + 1));
        strcpy(local_conf.unzip, val);
    }
}

int load_reader_config() {
    local_conf.hyphen = 1;
    local_conf.widen = 0;
    local_conf.filename = NULL;
    local_conf.unzip = NULL;
    local_conf.enc[0] = '\0';

    // TODO: detect portable/not
    char conf_path[1024];
    get_app_directory(conf_path, 1024);
    append_path(conf_path, 1024, "reader.conf");

    log_message("main.log", "Config file path: %s\n", conf_path);
    int res = load_config(conf_path, config_cb);

    return res == CONFIG_SUCCESS ? BOOK_SUCCESS : BOOK_INVALID_FILE;
}

int process_app_arg(char *arg) {
    if (arg == NULL) {
        return BOOK_INVALID_ARG;
    }
    log_message("main.log", "APP ARG: [%s]\n", arg);

    if (*arg == '-') {
        // option
        log_message("main.log", "Option: %c\n", arg[1]);
        if (arg[1] == 'e') {
            strncpy(local_conf.enc, &arg[2], 32);
            if (! is_encoding_supported(&arg[2])) {
                log_error("main.log", 1, "Encoding is not supported: %s\n", &arg[2]);
            } else {
                local_conf.enc[31] = '\0';
            }
        } else {
            log_error("main.log", 1, "Invalid option: %s\n", arg);
        }
    } else {
        // filename - get only the latest one
        log_message("main.log", "File name: %s\n", arg);
        if (local_conf.filename != NULL) {
            free(local_conf.filename);
            local_conf.filename = (char*)malloc(sizeof(char) * (strlen(arg) + 1));
            if (local_conf.filename == NULL) {
                return BOOK_NO_MEMORY;
            }
            strcpy(local_conf.filename, arg);
        }
    }

    return BOOK_SUCCESS;
}

const struct reader_conf* get_reader_options() {
    return &local_conf;
}
