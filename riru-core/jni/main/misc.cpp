#include <cstdio>
#include <cstring>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <sys/stat.h>
#include <cstdlib>

#include "wrap.h"

ssize_t fdgets(char *buf, const size_t size, int fd) {
    ssize_t len = 0;
    buf[0] = '\0';
    while (len < size - 1) {
        ssize_t ret = read(fd, buf + len, 1);
        if (ret < 0)
            return -1;
        if (ret == 0)
            break;
        if (buf[len] == '\0' || buf[len++] == '\n') {
            break;
        }
    }
    buf[len] = '\0';
    buf[size - 1] = '\0';
    return len;
}

int get_proc_name(int pid, char *name, size_t _size) {
    int fd;
    ssize_t __size;

    char buf[1024];
    snprintf(buf, sizeof(buf), "/proc/%d/cmdline", pid);
    if (access(buf, R_OK) == -1 || (fd = open(buf, O_RDONLY)) == -1)
        return 1;
    if ((__size = fdgets(buf, sizeof(buf), fd)) == 0) {
        snprintf(buf, sizeof(buf), "/proc/%d/comm", pid);
        close(fd);
        if (access(buf, R_OK) == -1 || (fd = open(buf, O_RDONLY)) == -1)
            return 1;
        __size = fdgets(buf, sizeof(buf), fd);
    }
    close(fd);

    if (__size < _size) {
        strncpy(name, buf, static_cast<size_t>(__size));
        name[__size] = '\0';
    } else {
        strncpy(name, buf, _size);
        name[_size] = '\0';
    }

    return 0;
}

void *memsearch(const uintptr_t addr_start, const uintptr_t addr_end, const void *s, size_t size) {
    uintptr_t _addr_start = addr_start;
    while (1) {
        if (_addr_start + size >= addr_end)
            return NULL;

        if (memcmp((const void *) _addr_start, s, size) == 0)
            return (void *) _addr_start;

        _addr_start += 1;
    }
}