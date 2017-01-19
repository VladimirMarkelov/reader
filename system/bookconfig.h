#ifndef BOOKCONFIG_H_20170117
#define BOOKCONFIG_H_20170117

struct reader_conf {
    int hyphen;
    int widen;
    char enc[32];
    char *filename;
    char *unzip;
};

int load_reader_config();
int process_app_arg(char *arg);
const struct reader_conf* get_reader_options();

#endif // BOOKCONFIG_H_20170117
