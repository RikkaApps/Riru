#ifndef _WRAP_H
#define _WRAP_H

#include <dirent.h>

__attribute__((visibility("hidden")))
DIR* _opendir(const char *path);

__attribute__((visibility("hidden")))
struct dirent *_readdir(DIR* dir);

#endif // _WRAP_H
