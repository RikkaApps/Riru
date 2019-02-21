#ifndef _MISC_H
#define _MISC_H

#include <string>
#include <sys/types.h>

ssize_t fdgets(char *buf, size_t size, int fd);

int get_proc_name(int pid, char *name, size_t size);

#endif // _MISC_H
