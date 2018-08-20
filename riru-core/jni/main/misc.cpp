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

int get_proc_name(int pid, char* name, size_t _size) {
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

unsigned char* memsearch(const unsigned char *addr_start, const unsigned char *addr_end,
                         const unsigned char *s, size_t size) {
    unsigned char* _addr_start = (unsigned char*) addr_start;
    while (1) {
        if (_addr_start + size >= addr_end)
            return NULL;

        if (memcmp(_addr_start, s, size) == 0)
            return _addr_start;

        _addr_start += 1;
    }
}

int mkdirs(const char *pathname, mode_t mode) {
    char *path = strdup(pathname), *p;
    errno = 0;
    for (p = path + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(path, mode) == -1) {
                if (errno != EEXIST)
                    return -1;
            }
            *p = '/';
        }
    }
    if (mkdir(path, mode) == -1) {
        if (errno != EEXIST)
            return -1;
    }
    free(path);
    return 0;
}

int switch_mnt_ns(int pid) {
    char mnt[32];
    snprintf(mnt, sizeof(mnt), "/proc/%d/ns/mnt", pid);
    if (access(mnt, R_OK) == -1) return -1; // Maybe process died..

    int fd, res;
    fd = _open(mnt, O_RDONLY);
    if (fd < 0) return -1;
    // Switch to its namespace
    res = _setns(fd, 0);
    close(fd);
    return res;
}

int read_namespace(const int pid, char* target, const size_t size) {
    char path[32];
    snprintf(path, sizeof(path), "/proc/%d/ns/mnt", pid);
    if (access(path, R_OK) == -1)
        return 1;
    _readlink(path, target, size);
    return 0;
}

int ensure_dir(const char *path, mode_t mode) {
    if (access(path, R_OK) == -1)
        return _mkdirs(path, mode);

    return 0;
}