#include <sys/mman.h>
#include <dlfcn.h>
#include <dl.h>
#include <link.h>
#include <string>
#include <magisk.h>
#include <android_prop.h>
#include "hide_utils.h"
#include "wrap.h"
#include "logging.h"
#include "module.h"
#include "entry.h"
#include <iostream>
#include <elf_util.h>

namespace Hide {
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

            static bool setup(const SandHook::ElfImg &linker) {
                ctor = MemFunc{.data = {.p = reinterpret_cast<void *>(linker.getSymbAddress(
                        "__dl__ZN18ProtectedDataGuardC2Ev")),
                        .adj = 0}}
                        .f;
                dtor = MemFunc{.data = {.p = reinterpret_cast<void *>(linker.getSymbAddress(
                        "__dl__ZN18ProtectedDataGuardD2Ev")),
                        .adj = 0}}
                        .f;
                return ctor != nullptr && dtor != nullptr;
            }

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

        ProtectedDataGuard::FuncType ProtectedDataGuard::ctor = nullptr;
        ProtectedDataGuard::FuncType ProtectedDataGuard::dtor = nullptr;

        void *(*solist_get_head)() = nullptr;

        void *(*solist_get_somain)() = nullptr;

        bool (*solist_remove_soinfo)(void *) = nullptr;

        const char *(*soinfo_get_realpath)(void *) = nullptr;

        uintptr_t *solist_head = nullptr;
        uintptr_t somain = 0;
        size_t solist_next_offset = 0;

        bool init_solist() {
            solist_head = static_cast<uintptr_t *>(solist_get_head());
            somain = reinterpret_cast<uintptr_t>(solist_get_somain());
            for (size_t i = 0; i < 1024 / sizeof(void *); i++) {
                if (*(uintptr_t *) ((uintptr_t) solist_head + i * sizeof(void *)) == somain) {
                    solist_next_offset = i * sizeof(void *);
                    return true;
                }
            }
            return false;
        }

        const auto initialized = []() {
            SandHook::ElfImg linker(GetLinkerPath());
            return ProtectedDataGuard::setup(linker) &&
                   (solist_get_head = reinterpret_cast<decltype(solist_get_head)>(linker.getSymbAddress(
                           "__dl__Z15solist_get_headv"))) != nullptr &&
                   (solist_get_somain = reinterpret_cast<decltype(solist_get_somain)>(linker.getSymbAddress(
                           "__dl__Z17solist_get_somainv"))) != nullptr &&
                   (soinfo_get_realpath = reinterpret_cast<decltype(soinfo_get_realpath)>(linker.getSymbAddress(
                           "__dl__ZNK6soinfo12get_realpathEv"))) != nullptr &&
                   (solist_remove_soinfo = reinterpret_cast<decltype(solist_remove_soinfo)>(linker.getSymbAddress(
                           "__dl__Z20solist_remove_soinfoP6soinfo"))) != nullptr && init_solist();
        }();

        std::vector<void *> linker_get_solist() {
            std::vector<void *> linker_solist{solist_head};

            uintptr_t sonext = *(uintptr_t *) ((uintptr_t) solist_head + solist_next_offset);
            while (sonext) {
                linker_solist.push_back((void *) sonext);
                sonext = *(uintptr_t *) ((uintptr_t) sonext + solist_next_offset);
            }

            return linker_solist;
        }

        void HidePathsFromSolist(const std::vector<const char *> &names) {
            if (!initialized) {
                LOGW("not initialized");
                return;
            }
            ProtectedDataGuard g;
            auto list = linker_get_solist();
            for (const auto &soinfo : linker_get_solist()) {
                const char *real_path = soinfo_get_realpath(soinfo);
                if (real_path != nullptr) {
                    for (const auto &path : names) {
                        if (strcmp(path, real_path) == 0) {
                            LOGD("remove soinfo %s", real_path);
                            solist_remove_soinfo(soinfo);
                            break;
                        }
                    }
                }
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
        std::vector<const char *> map_to_remove{};
        std::vector<const char *> so_to_remove{};
        for (auto module : Modules::Get()) {
            if (strcmp(module->id, MODULE_NAME_CORE) == 0) {
                if (Entry::IsSelfUnloadAllowed()) {
                    LOGD("don't hide self since it will be unloaded");
                } else {
                    map_to_remove.push_back(self_path.c_str());
                    so_to_remove.push_back(self_path.c_str());
                }
            } else if (module->supportHide) {
                if (!module->isLoaded()) {
                    LOGD("%s is unloaded", module->id);
                } else {
                    if (module->apiVersion <= 24) {
                        LOGW("%s is too old to hide so", module->id);
                    } else {
                        so_to_remove.push_back(self_path.c_str());
                    }
                    map_to_remove.push_back(module->path);
                }
            } else {
                LOGD("module %s does not support hide", module->id);
            }
        }
        if (!so_to_remove.empty() && solist) Hide::HidePathsFromSolist(so_to_remove);
        if (!map_to_remove.empty() && maps) Hide::HidePathsFromMaps(map_to_remove);
    }
}
