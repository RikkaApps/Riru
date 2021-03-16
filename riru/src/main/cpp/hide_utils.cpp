#include <sys/mman.h>
#include <dlfcn.h>
#include <dl.h>
#include <link.h>
#include <string>
#include <magisk.h>
#include <dobby.h>
#include <android_prop.h>
#include "hide_utils.h"
#include "wrap.h"
#include "logging.h"
#include "module.h"
#include <bionic_linker_restriction.h>
#include "entry.h"
#include <iostream>

namespace Hide {
    class ProtectedDataGuard {

    public:
        ProtectedDataGuard() {
            if (ctor != nullptr)
                (this->*ctor)();
        }

        ~ProtectedDataGuard() {
            if (dtor != nullptr)
                (this->*dtor)();
        }

    public:
        ProtectedDataGuard(const ProtectedDataGuard &) = delete;

        void operator=(const ProtectedDataGuard &) = delete;

    private:
        using FuncType = void (ProtectedDataGuard::*)();

        static FuncType ctor;
        static FuncType dtor;

        union MemFunc {
            FuncType f;

            struct {
                void *p;
                std::ptrdiff_t adj;
            } data;
        };
    };

    ProtectedDataGuard::FuncType ProtectedDataGuard::ctor = MemFunc{.data = {.p = DobbySymbolResolver(
            nullptr, "__dl__ZN18ProtectedDataGuardC2Ev"),
            .adj = 0}}
            .f;
    ProtectedDataGuard::FuncType ProtectedDataGuard::dtor = MemFunc{.data = {.p = DobbySymbolResolver(
            nullptr, "__dl__ZN18ProtectedDataGuardD2Ev"),
            .adj = 0}}
            .f;

    namespace {
        const char *GetLinkerPath() {
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

        using solist_remove_soinfo_t = bool(soinfo_t soinfo);

        auto solist_remove_soinfo = (solist_remove_soinfo_t *)
                DobbySymbolResolver(GetLinkerPath(), "__dl__Z20solist_remove_soinfoP6soinfo");

        [[maybe_unused]] auto dummy = []() {
            linker_iterate_soinfo([](soinfo_t soinfo) {
                [[maybe_unused]] const char *real_path = linker_soinfo_get_realpath(soinfo);
                return 0;
            });
            return nullptr;
        }();

        void HidePathsFromSolist(const std::vector<const char *> &names) {
            if (solist_remove_soinfo != nullptr) {
                static std::vector<const char *> const *_names;
                static std::vector<soinfo_t> soinfo_to_remove;

                _names = &names;

                static uintptr_t pagesize = sysconf(_SC_PAGE_SIZE);

                linker_iterate_soinfo([](soinfo_t soinfo) {
                    const char *real_path = linker_soinfo_get_realpath(soinfo);
                    if (real_path != nullptr) {
                        for (const auto &_path : *_names) {
                            if (strcmp(_path, real_path) == 0) {
                                LOGD("remove soinfo %s", real_path);
                                soinfo_to_remove.emplace_back(soinfo);
                                break;
                            }
                        }
                    }
                    return 0;
                });

                ProtectedDataGuard g;

                for (auto soinfo : soinfo_to_remove) {
                    solist_remove_soinfo(soinfo);
                    LOGD("remove soinfo %p", soinfo);
                }
                LOGD("after removed");
                linker_iterate_soinfo([](soinfo_t soinfo) {
                    const char *real_path = linker_soinfo_get_realpath(soinfo);
                    LOGD("rested soinfo %s", real_path);
                    return 0;
                });
            } else {
                LOGE("cannot find solist_remove_soinfo");
            }
        }

        void HidePathsFromMaps(const std::vector<const char *> &names) {
            auto hide_lib_path = Magisk::GetPathForSelfLib("libriruhide.so");

            // load riruhide.so and run the hide
            LOGD("dlopen libriruhide");
            auto handle = dlopen_ext(hide_lib_path.c_str(), 0);
            if (!handle) {
                LOGE("dlopen %s failed: %s", hide_lib_path.c_str(), dlerror());
                return;
            }
            using riru_hide_t = int(const char *const *names, int names_count);
            auto *riru_hide = (riru_hide_t *) dlsym(handle, "riru_hide");
            if (!riru_hide) {
                LOGE("dlsym failed: %s", dlerror());
                return;
            }

            LOGD("do hide");
            riru_hide(&names[0], names.size());

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
                LOGV("%" PRIxPTR"-%" PRIxPTR" %s %ld %s", start, end, maps_tmp->perm,
                     maps_tmp->offset,
                     maps_tmp->pathname);
                munmap((void *) start, size);
            }
            pmparser_free(maps);
        }
    }

    void DoHide(bool solist, bool maps) {
        auto self_path = Magisk::GetPathForSelfLib("libriru.so");
        auto modules = Modules::Get();
        std::vector<const char *> names{};
        for (auto module : Modules::Get()) {
            if (strcmp(module->id, MODULE_NAME_CORE) == 0) {
                if (Entry::IsSelfUnloadAllowed()) {
                    LOGD("don't hide self since it will be unloaded");
                }
                names.push_back(self_path.c_str());
            } else if (module->apiVersion <= 24) {
                LOGD("%s is too old to hide", module->id);
            } else if (module->supportHide) {
                if (!module->isLoaded()) {
                    LOGD("%s is unloaded", module->id);
                } else {
                    names.push_back(module->path);
                }
            } else {
                LOGD("module %s does not support hide", module->id);
            }
        }
        if (!names.empty()) {
            if (solist) Hide::HidePathsFromSolist(names);
            if (maps) Hide::HidePathsFromMaps(names);
        }
    }
}
