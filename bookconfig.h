#ifndef BOOKCONFIG_H_20170117
#define BOOKCONFIG_H_20170117

struct reader_conf {
    int hyphen;
    int widen;
};

int load_reader_config(struct reader_conf *conf);

#endif // BOOKCONFIG_H_20170117
