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

using namespace std;
using namespace std::string_literals;

namespace Magisk {

    static const char *path_;

    const char *GetPath() {
        return path_;
    }

    void SetPath(const char *path) {
        if (path) path_ = strdup(path);
    }

    string GetPathForSelf(const char *name) {
        string str;
        str = string(GetPath()) + "/.magisk/modules/riru-core/"s + name;
        return str;
    }

    string GetPathForSelfLib(const char *name) {
        string str;
#ifdef __LP64__
        str = string(GetPath()) + "/.magisk/modules/riru-core/lib64/"s + name;
#else
        str = string(GetPath()) + "/.magisk/modules/riru-core/lib/"s + name;
#endif
        return str;
    }

    void ForEachModule(const function<void(const char *)> &fn) {
        auto root = GetPath();
        if (!root) return;

        char buf[PATH_MAX];
        strcpy(buf, root);
        strcat(buf, "/.magisk/modules");
        auto modules_end = buf + strlen(buf);

        DIR *dir;
        struct dirent *entry;

        if (!(dir = opendir(buf))) return;

        while ((entry = readdir(dir))) {
            if (entry->d_type != DT_DIR) continue;
            if (entry->d_name[0] == '.') continue;

            *modules_end = '\0';
            strcat(buf, "/");
            strcat(buf, entry->d_name);

            auto end = buf + strlen(buf);
            strcat(buf, "/disable");
            if (access(buf, F_OK) == 0) continue;
            *end = '\0';

            strcat(buf, "/remove");
            if (access(buf, F_OK) == 0) continue;
            *end = '\0';

            fn(buf);
        }

        closedir(dir);
    }
}