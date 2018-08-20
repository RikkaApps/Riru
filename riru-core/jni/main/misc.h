#ifndef _MISC_H
#define _MISC_H

#include <string>
#include <sys/types.h>

ssize_t fdgets(char *buf, const size_t size, int fd);
int get_proc_name(int pid, char* name, size_t size);
unsigned char* memsearch(const unsigned char *addr_start, const unsigned char *addr_end,
                         const unsigned char *s, size_t size);
int mkdirs(const char *pathname, mode_t mode);
int switch_mnt_ns(int pid);
int read_namespace(const int pid, char* target, const size_t size);
int ensure_dir(const char *path, mode_t mode);

#endif // _MISC_H
