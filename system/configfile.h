#ifndef CONFIGFILE_H_20170116
#define CONFIGFILE_H_20170116

#define CONFIG_SUCCESS 0
#define CONFIG_NO_FILE 1
#define CONFIG_INVALID 2
#define CONFIG_NO_MEMORY 3

typedef void (*config_callback) (const char*, const char*);

int load_config(char *filename, config_callback cb);

#endif // CONFIGFILE_H_20170116
