#ifndef _MISC_H
#define _MISC_H

#include <string>
#include <sys/types.h>

ssize_t fdgets(char *buf, size_t size, int fd);
ssize_t get_self_cmdline(char *cmdline);

#endif // _MISC_H
