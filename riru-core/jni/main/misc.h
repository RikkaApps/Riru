#ifndef _MISC_H
#define _MISC_H

#include <string>
#include <sys/types.h>

ssize_t fdgets(char *buf, const size_t size, int fd);
int get_proc_name(int pid, char *name, size_t size);
void *memsearch(const uintptr_t addr_start, const uintptr_t addr_end, const void *s, size_t size);

#endif // _MISC_H
