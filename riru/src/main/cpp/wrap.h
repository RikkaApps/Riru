#ifndef _WRAP_H
#define _WRAP_H

#include <dirent.h>

DIR *_opendir(const char *path);

struct dirent *_readdir(DIR *dir);

int _mprotect(void *addr, size_t size, int prot);

void *_mmap(void *addr, size_t size, int prot, int flags, int fd, off_t offset);

#endif // _WRAP_H
