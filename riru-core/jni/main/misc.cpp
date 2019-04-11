#include <cstdio>
#include <cstring>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <sys/stat.h>
#include <cstdlib>

#include "wrap.h"

ssize_t fdgets(char *buf, size_t size, int fd) {
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

ssize_t get_self_cmdline(char *cmdline) {
    int fd;
    ssize_t size;

    char buf[PATH_MAX];
    snprintf(buf, sizeof(buf), "/proc/self/cmdline");
    if (access(buf, R_OK) == -1 || (fd = open(buf, O_RDONLY)) == -1)
        return 1;

    if ((size = read(fd, cmdline, ARG_MAX)) > 0) {
        for (ssize_t i = 0; i < size - 1; ++i) {
            if (cmdline[i] == 0)
                cmdline[i] = ' ';
        }
    } else {
        cmdline[0] = 0;
    }

    close(fd);
    return size;
}