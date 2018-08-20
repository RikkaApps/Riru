#ifndef _WRAP_H
#define _WRAP_H

#include <dirent.h>

DIR* _opendir(const char *path);
struct dirent *_readdir(DIR* dir);

#endif // _WRAP_H
