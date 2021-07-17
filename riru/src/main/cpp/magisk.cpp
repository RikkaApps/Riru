#include <cstdio>
#include <climits>
#include <functional>
#include <string>
#include <malloc.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <misc.h>
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

    void ForEachModule(const std::function<void(const char *)> &fn) {
        const auto &root = GetPath();
        if (root.empty()) return;

        BuffString<PATH_MAX> buf;
        buf += root;
        buf += "/.magisk/modules";
        auto modules_end = buf.size();

        DIR *dir;
        struct dirent *entry;

        if (!(dir = opendir(buf))) return;

        while ((entry = readdir(dir))) {
            if (entry->d_type != DT_DIR) continue;
            if (entry->d_name[0] == '.') continue;

            buf.size(modules_end);

            buf += "/";
            buf += entry->d_name;

            auto end = buf.size();

            buf += "/disable";
            if (access(buf, F_OK) == 0) continue;
            buf.size(end);

            buf += "/remove";
            if (access(buf, F_OK) == 0) continue;
            buf.size(end);

            fn(buf);
        }

        closedir(dir);
    }
}  // namespace magisk
