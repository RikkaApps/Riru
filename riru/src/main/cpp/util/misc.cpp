#include <cstdio>
#include <cstring>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <sys/stat.h>
#include <cstdlib>
#include <cctype>

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

ssize_t get_self_cmdline(char *cmdline, char zero_replacement) {
    int fd;
    ssize_t size;

    char buf[PATH_MAX];
    snprintf(buf, sizeof(buf), "/proc/self/cmdline");
    if (access(buf, R_OK) == -1 || (fd = open(buf, O_RDONLY)) == -1)
        return 1;

    if ((size = read(fd, cmdline, ARG_MAX)) > 0) {
        for (ssize_t i = 0; i < size - 1; ++i) {
            if (cmdline[i] == 0)
                cmdline[i] = zero_replacement;
        }
    } else {
        cmdline[0] = 0;
    }

    close(fd);
    return size;
}

char *trim(char *str) {
    size_t len = 0;
    char *frontp = str;
    char *endp = nullptr;

    if (str == nullptr) { return nullptr; }
    if (str[0] == '\0') { return str; }

    len = strlen(str);
    endp = str + len;

    /* Move the front and back pointers to address the first non-whitespace
     * characters from each end.
     */
    while (isspace((unsigned char) *frontp)) { ++frontp; }
    if (endp != frontp) {
        while (isspace((unsigned char) *(--endp)) && endp != frontp) {}
    }

    if (str + len - 1 != endp)
        *(endp + 1) = '\0';
    else if (frontp != str && endp == frontp)
        *str = '\0';

    /* Shift the string so that it starts at str so that if it's dynamically
     * allocated, we can still free it on the returned pointer.  Note the reuse
     * of endp to mean the front of the string buffer now.
     */
    endp = str;
    if (frontp != str) {
        while (*frontp) { *endp++ = *frontp++; }
        *endp = '\0';
    }

    return str;
}

int get_prop(const char *file, const char *key, char *value) {
    int fd = open(file, O_RDONLY);
    if (fd == -1)
        return -1;

    char buf[512];
    char *trimmed, *_key, *_value;
    while (fdgets(buf, 512, fd) > 0) {
        if (*buf == 0 || *buf == '#' || !strstr(buf, "="))
            continue;

        trimmed = trim(buf);

        _value = trimmed;
        _key = strsep(&_value, "=");
        if (!_key)
            continue;

        if (strcmp(key, _key) == 0) {
            strcpy(value, _value);
            close(fd);
            return static_cast<int>(strlen(value));
        }
    }
    close(fd);
    return -1;
}

static ssize_t read_eintr(int fd, void *out, size_t len) {
    ssize_t ret;
    do {
        ret = read(fd, out, len);
    } while (ret < 0 && errno == EINTR);
    return ret;
}

int read_full(int fd, void *out, size_t len) {
    while (len > 0) {
        ssize_t ret = read_eintr(fd, out, len);
        if (ret <= 0) {
            return -1;
        }
        out = (void *) ((uintptr_t) out + ret);
        len -= ret;
    }
    return 0;
}

int write_full(int fd, const void *buf, size_t count) {
    while (count > 0) {
        ssize_t size = write(fd, buf, count < SSIZE_MAX ? count : SSIZE_MAX);
        if (size == -1) {
            if (errno == EINTR)
                continue;
            else
                return -1;
        }
        buf = (const void *) ((uintptr_t) buf + size);
        count -= size;
    }
    return 0;
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