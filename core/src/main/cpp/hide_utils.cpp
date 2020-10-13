#include <sys/mman.h>
#include <dlfcn.h>
#include "hide_utils.h"
#include "wrap.h"
#include "logging.h"

#ifndef DEBUG_APP
#ifdef __LP64__
#define LIB_PATH "/system/lib64/"
#else
#define LIB_PATH "/system/lib/"
#endif
#else
#define LIB_PATH "/data/data/moe.riru.manager/lib/"
#endif

namespace hide {

    void hide_modules(const char **names, int names_count) {
        // load riruhide.so and run the hide
        LOGD("dlopen libriruhide");
        auto handle = dlopen(LIB_PATH "libriruhide.so", 0);
        if (!handle) {
            LOGE("dlopen %s failed: %s", LIB_PATH "libriruhide.so", dlerror());
            return;
        }
        using riru_hide_t = int(const char **names, int names_count);
        auto *riru_hide = (riru_hide_t *) dlsym(handle, "riru_hide");
        if (!riru_hide) {
            LOGE("dlsym failed: %s", dlerror());
            return;
        }

        LOGD("do hide");
        riru_hide(names, names_count);

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