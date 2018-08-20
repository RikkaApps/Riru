#ifndef _WRAP_H
#define _WRAP_H

#include <dirent.h>

int _mount(const char *source, const char *target, unsigned long flags);
int _umount2(const char *target, int flags);
int _mkdir(const char *path, mode_t mode);
int _mkdirs(const char* path, mode_t mode);
DIR* _opendir(const char *path);
struct dirent *_readdir(DIR* dir);
int _pipe2(int fds[2], int flags);
pid_t _fork();
int _dup2(int old_fd, int new_fd);
int _setns(int fd, int ns_type);
int _open(const char* path, int flags);
ssize_t _write(int fd, const void* buf, size_t count);
ssize_t _readlink(const char* path, char* buf, size_t buf_size);
int _unshare(int flags);
int _execvp(const char* file, char* const* argv);
int _chmod(const char* path, mode_t mode);
int _rename(const char* old_path, const char* new_path);

#endif // _WRAP_H
