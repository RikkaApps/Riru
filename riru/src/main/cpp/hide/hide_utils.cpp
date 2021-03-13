#include <sys/mman.h>
#include <dlfcn.h>
#include <dl.h>
#include <link.h>
#include <string>
#include <magisk.h>
#include "hide_utils.h"
#include "wrap.h"
#include "logging.h"

namespace hide {

    struct hide_data {
        const char **paths;
        int paths_count;
    };

    static int callback(struct dl_phdr_info *info, size_t size, void *_data) {
        auto data = (hide_data *) _data;

        for (int i = 0; i < data->paths_count; i++) {
            if (strcmp(data->paths[i], info->dlpi_name) == 0) {
                memset((void *) info->dlpi_name, 0, strlen(data->paths[i]));
                LOGD("hide %s from dl_iterate_phdr", data->paths[i]);
                return 0;
            }
        }
        return 0;
    }

    void hide_modules(const char **paths, int paths_count) {
        auto data = hide_data{
                .paths = paths,
                .paths_count = paths_count
        };
        dl_iterate_phdr(callback, &data);

        auto hide_lib_path = Magisk::GetPathForSelfLib("libriruhide.so");

        // load riruhide.so and run the hide
        LOGD("dlopen libriruhide");
        auto handle = dlopen_ext(hide_lib_path.c_str(), 0);
        if (!handle) {
            LOGE("dlopen %s failed: %s", hide_lib_path.c_str(), dlerror());
            return;
        }
        using riru_hide_t = int(const char **names, int names_count);
        auto *riru_hide = (riru_hide_t *) dlsym(handle, "riru_hide");
        if (!riru_hide) {
            LOGE("dlsym failed: %s", dlerror());
            return;
        }

        LOGD("do hide");
        riru_hide(paths, paths_count);

        // cleanup riruhide.so
        LOGD("dlclose");
        if (dlclose(handle) != 0) {
            LOGE("dlclose failed: %s", dlerror());
            return;
        }

        procmaps_iterator *maps = pmparser_parse(-1);
        if (maps == nullptr) {
            LOGE("cannot parse the memory map");
            return;
        }

        procmaps_struct *maps_tmp;
        while ((maps_tmp = pmparser_next(maps)) != nullptr) {
            if (!strstr(maps_tmp->pathname, "/libriruhide.so")) continue;

            auto start = (uintptr_t) maps_tmp->addr_start;
            auto end = (uintptr_t) maps_tmp->addr_end;
            auto size = end - start;
            LOGV("%" PRIxPTR"-%" PRIxPTR" %s %ld %s", start, end, maps_tmp->perm, maps_tmp->offset, maps_tmp->pathname);
            munmap((void *) start, size);
        }
        pmparser_free(maps);
    }
}