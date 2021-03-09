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
#include "status.h"

using namespace std;
using namespace std::string_literals;

namespace Magisk {

    const char *GetPath() {
        static char *path = nullptr;
        if (path) return path;
        int32_t size;
        Status::ReadMagiskTmpfsPath(path, size);
        return path;
    }

    string GetPathForSelf(const char *name) {
        string str;
        str = string(GetPath()) + "/.magisk/modules/riru-core/"s + name;
        return str;
    }

    void ForEachRiruModuleLibrary(const function<void(string_view, const char*)> &fn) {
        auto root = GetPath();
        if (!root) return;

        char buf[PATH_MAX];
        strcpy(buf, root);
        strcat(buf, "/.magisk/modules");
        auto modules_end = buf + strlen(buf);

        DIR *dir, *lib_dir;
        struct dirent *entry, *lib_entry;

        if (!(dir = opendir(buf))) return;

        while ((entry = readdir(dir))) {
            if (entry->d_type != DT_DIR) continue;
            if (entry->d_name[0] == '.') continue;

            *modules_end = '\0';
            strcat(buf, "/");
            strcat(buf, entry->d_name);
            strcat(buf, "/riru/lib");
#ifdef __LP64__
            strcat(buf, "64");
#endif

            if (access(buf, F_OK) == -1) {
                continue;
            }

            LOGI("Magisk module %s is a Riru module", entry->d_name);

            strcat(buf, "/");

            if ((lib_dir = opendir(buf))) {
                while ((lib_entry = readdir(lib_dir))) {
                    if (lib_entry->d_type != DT_REG) continue;

                    auto end = buf + strlen(buf);
                    strcat(buf, lib_entry->d_name);

                    auto id = string(entry->d_name) + "/" + (lib_entry->d_name);
                    fn(id, buf);

                    *end = '\0';
                }
                closedir(lib_dir);
            }
        }

        closedir(dir);
    }
}