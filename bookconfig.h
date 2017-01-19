#ifndef BOOKCONFIG_H_20170117
#define BOOKCONFIG_H_20170117

struct reader_conf {
    int hyphen;
    int widen;
    char enc[32];
    char *filename;
};

int load_reader_config(struct reader_conf *conf);
int process_app_arg(char *arg, struct reader_conf *conf);

#endif // BOOKCONFIG_H_20170117
