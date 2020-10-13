#ifndef _MISC_H
#define _MISC_H

#include <sys/types.h>

ssize_t fdgets(char *buf, size_t size, int fd);
ssize_t get_self_cmdline(char *cmdline, char zero_replacement);
char *trim(char *str);
int get_prop(const char *file, const char *key, char *value);
int read_full(int fd, void *buf, size_t count);
int write_full(int fd, const void *buf, size_t count);
int mkdirs(const char *pathname, mode_t mode);

#endif // _MISC_H
