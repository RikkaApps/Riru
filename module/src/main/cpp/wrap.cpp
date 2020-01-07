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