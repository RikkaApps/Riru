#include <cstdio>
#include <climits>
#include <functional>
#include <string>
#include <malloc.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <logging.h>
#include <config.h>
#include <rirud.h>
#include "buff_string.h"

using namespace std::string_literals;

namespace magisk {

    static std::string path;

    const auto &GetPath() {
        return path;
    }

    void SetPath(const char *p) {
        if (p) path = p;
    }

    std::string GetPathForSelf(const char *name) {
        return GetPath() + "/.magisk/modules/riru-core/"s + name;
    }

    std::string GetPathForSelfLib(const char *name) {
#ifdef __LP64__
        return GetPath() + "/.magisk/modules/riru-core/lib64/"s + name;
#else
        return GetPath() + "/.magisk/modules/riru-core/lib/"s + name;
#endif
    }
}  // namespace magisk
