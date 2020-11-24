#include <cstdint>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <cstring>
#include <dirent.h>
#include <unistd.h>
#include <sched.h>
#include <fcntl.h>
#include <cstdio>

#include "logging.h"
#include "misc.h"

DIR *_opendir(const char *path) {
    DIR *d = opendir(path);
    if (d == NULL) {
        PLOGE("opendir %s", path);
    }
    return d;
}

struct dirent *_readdir(DIR *dir) {
    errno = 0;
    struct dirent *ent = readdir(dir);
    if (errno && ent == NULL) {
        PLOGE("readdir");
    }
    return ent;
}

int _mprotect(void *addr, size_t size, int prot) {
    int res = mprotect(addr, size, prot);
    if (res != 0) {
        PLOGE("mprotect");
    }
    return res;
}

void *_mmap(void *addr, size_t size, int prot, int flags, int fd, off_t offset) {
    auto res = mmap(addr, size, prot, flags, fd, offset);
    if (res == MAP_FAILED) {
        PLOGE("mmap");
    }
    return res;
}