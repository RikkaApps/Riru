#include <stdint.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sched.h>
#include <fcntl.h>
#include <stdio.h>

#include "logging.h"
#include "misc.h"

int _mount(const char *source, const char *target, unsigned long flags)
{
    int res = mount(source, target, NULL, flags, NULL);
    if (res == -1)
    {
        PLOGE("mount %s->%s", source, target);
        if (errno == ENOENT)
        {
            if (access(source, R_OK) == -1)
                LOGE("%s not exist", source);

            if (access(target, R_OK) == -1)
                LOGE("%s not exist", target);
        }
    }

    return res;
}

int _umount2(const char *target, int flags)
{
    int res = umount2(target, flags);
    if (res == -1)
    {
        PLOGE("umount2 %s", target);
        if (errno == ENOENT)
        {
            if (access(target, R_OK) == -1)
                LOGE("%s not exist", target);
        }
    }

    return res;
}

int _mkdir(const char* path, mode_t mode)
{
    int res = mkdir(path, mode);
    if (res == -1 && errno != EEXIST)
        PLOGE("mkdir %s %o", path, mode);

    return res;
}

int _mkdirs(const char* path, mode_t mode)
{
    int res = mkdirs(path, mode);
    if (res == -1 && errno != EEXIST)
        PLOGE("mkdirs %s %o", path, mode);

    return res;
}

DIR* _opendir(const char *path)
{
    DIR *d = opendir(path);
    if (d == NULL) {
        PLOGE("opendir %s", path);
    }
    return d;
}

struct dirent *_readdir(DIR *dir)
{
    errno = 0;
    struct dirent *ent = readdir(dir);
    if (errno && ent == NULL) {
        PLOGE("readdir");
    }
    return ent;
}

int _pipe2(int fds[2], int flags)
{
    int res = pipe2(fds, flags);
    if (res == -1) {
        PLOGE("pipe2");
    }
    return res;
}

pid_t _fork() {
    int res = fork();
    if (res == -1) {
        PLOGE("fork");
    }
    return res;
}

int _dup2(int old_fd, int new_fd)
{
    int res = dup2(old_fd, new_fd);
    if (res == -1) {
        PLOGE("dup2");
    }
    return res;
}

int _setns(int fd, int ns_type)
{
    int res = setns(fd, ns_type);
    if (res == -1) {
        PLOGE("setns");
    }
    return res;
}

int _open(const char* path, int flags)
{
    int res = open(path, flags);
    if (res == -1) {
        PLOGE("open: %s", path);
    }
    return res;
}

ssize_t _write(int fd, const void* buf, size_t count)
{
    ssize_t res = write(fd, buf, count);
    if (res == -1) {
        PLOGE("write");
    }
    return res;
}

ssize_t _readlink(const char* path, char* buf, size_t buf_size)
{
    ssize_t res = readlink(path, buf, buf_size);
    if (res == -1) {
        PLOGE("readlink %s", path);
    } else {
        buf[res] = '\0';
    }
    return res;
}

int _unshare(int flags)
{
    int res = unshare(flags);
    if (res == -1) {
        PLOGE("unshare %d", flags);
    }
    return res;
}

int _execvp(const char* file, char* const* argv)
{
    int res = execvp(file, argv);
    if (res == -1) {
        PLOGE("execvp %s", file);
    }
    return res;
}

int _chmod(const char* path, mode_t mode)
{
    int res = chmod(path, mode);
    if (res == -1) {
        PLOGE("chmod %s %04o", path, mode);
    }
    return res;
}

int _rename(const char* old_path, const char* new_path)
{
    int res = rename(old_path, new_path);
    if (res == -1) {
        PLOGE("rename %s %s", old_path, new_path);
    }
    return res;
}