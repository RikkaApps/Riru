#include <sys/mman.h>
#include <dlfcn.h>
#include <dl.h>
#include <link.h>
#include <string>
#include <magisk.h>
//#include <dobby.h>
#include <android_prop.h>
#include "hide_utils.h"
#include "wrap.h"
#include "logging.h"
#include "module.h"
//#include "bionic_linker_restriction.h"
#include "entry.h"

namespace Hide {

    static const char *GetLinkerPath() {
#if __LP64__
        if (AndroidProp::GetApiLevel() >= 29) {
            return "/apex/com.android.runtime/bin/linker64";
        } else {
            return "/system/bin/linker64";
        }
#else
        if (AndroidProp::GetApiLevel() >= 29) {
            return "/apex/com.android.runtime/bin/linker";
        } else {
            return "/system/bin/linker";
        }
#endif
    }

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


    static void HidePathsFromSolist(const char **paths, int paths_count) {
        auto data = hide_data{
                .paths = paths,
                .paths_count = paths_count
        };
        dl_iterate_phdr(callback, &data);

        /*using solist_remove_soinfo_t = bool(soinfo_t soinfo);

        auto solist_remove_soinfo = (solist_remove_soinfo_t *)
                DobbySymbolResolver(GetLinkerPath(), "__dl__Z20solist_remove_soinfoP6soinfo");

        if (solist_remove_soinfo != nullptr) {
            static const char **_paths;
            static int _paths_count;
            static std::vector<soinfo_t> soinfo_to_remove;

            _paths = paths;
            _paths_count = paths_count;

            static uintptr_t pagesize = sysconf(_SC_PAGE_SIZE);

            linker_iterate_soinfo([](soinfo_t soinfo) {
                auto start = (((uintptr_t) soinfo - 1024 + pagesize - 1) & ~(pagesize - 1));
                if (mprotect((void *) start, 1024, PROT_READ | PROT_WRITE) != 0) {
                    PLOGE("mprotect %p (%p)", soinfo, (void *) start);
                } else {
                    LOGD("mprotect %p (%p)", soinfo, (void *) start);
                }

                const char *real_path = linker_soinfo_get_realpath(soinfo);
                if (real_path != nullptr) {
                    for (int i = 0; i < _paths_count; i++) {
                        if (strcmp(_paths[i], real_path) == 0) {
                            LOGD("remove soinfo %s", real_path);
                            soinfo_to_remove.emplace_back(soinfo);
                            break;
                        }
                    }
                }
                return 0;
            });

            for (auto soinfo : soinfo_to_remove) {
                solist_remove_soinfo(soinfo);
                LOGD("remove soinfo %p", soinfo);
            }
        } else {
            LOGE("cannot find solist_remove_soinfo");
        }*/
    }

    static void HidePathsFromMaps(const char **paths, int paths_count) {
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

    void DoHide(bool solist, bool maps) {
        auto self_path = Magisk::GetPathForSelfLib("libriru.so");
        auto modules = Modules::Get();
        auto names = (const char **) malloc(sizeof(char *) * modules.size());
        int names_count = 0;
        for (auto module : Modules::Get()) {
            if (strcmp(module->id, MODULE_NAME_CORE) == 0) {
                if (Entry::IsSelfUnloadAllowed()) {
                    LOGD("don't hide self since it will be unloaded");
                    continue;
                }
                names[names_count] = self_path.c_str();
            } else if (module->supportHide) {
                if (!module->isLoaded()) {
                    LOGD("%s is unloaded", module->id);
                    continue;
                }
                names[names_count] = module->path;
            } else {
                LOGD("module %s does not support hide", module->id);
                continue;
            }
            names_count += 1;
        }
        if (names_count > 0) {
            if (solist) Hide::HidePathsFromSolist(names, names_count);
            if (maps) Hide::HidePathsFromMaps(names, names_count);
        }
        free(names);
    }
}