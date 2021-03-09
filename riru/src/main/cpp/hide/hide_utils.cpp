#include <sys/mman.h>
#include <dlfcn.h>
#include <dl.h>
#include <string>
#include <magisk.h>
#include "hide_utils.h"
#include "wrap.h"
#include "logging.h"

namespace hide {

    void hide_modules(const char **paths, int paths_count) {
        const char *hide_lib_path;
#ifdef __LP64__
        hide_lib_path = Magisk::GetPathForSelf("lib64/libriruhide.so").c_str();
#else
        hide_lib_path = Magisk::GetPathForSelf("lib/libriruhide.so").c_str();
#endif

        // load riruhide.so and run the hide
        LOGD("dlopen libriruhide");
        auto handle = dl_dlopen(hide_lib_path, 0);
        if (!handle) {
            LOGE("dlopen %s failed: %s", hide_lib_path, dlerror());
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