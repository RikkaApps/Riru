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

        struct soinfo {
            soinfo *next() {
                return *(soinfo **) ((uintptr_t) this + solist_next_offset);
            }

            void next(soinfo *si) {
                *(soinfo **) ((uintptr_t) this + solist_next_offset) = si;
            }

            const char *get_realpath() {
                return get_realpath_sym ? get_realpath_sym(this) : nullptr;
            }

#ifdef __LP64__
            constexpr static size_t solist_next_offset = 0x28;
#else
            constexpr static size_t solist_next_offset = 0xa4;
#endif

            // since Android 6
            static const char *(*get_realpath_sym)(soinfo *);
        };

        const char *(*soinfo::get_realpath_sym)(soinfo *) = nullptr;

        soinfo *solist = nullptr;
        soinfo *sonext = nullptr;


        bool solist_remove_soinfo(soinfo *si) {
            soinfo *prev = nullptr, *trav;
            for (trav = solist; trav != nullptr; trav = trav->next()) {
                if (trav == si) {
                    break;
                }
                prev = trav;
            }

            if (trav == nullptr) {
                // si was not in solist
                LOGE("name \"%s\"@%p is not in solist!", si->get_realpath(), si);
                return false;
            }

            // prev will never be null, because the first entry in solist is
            // always the static libdl_info.
            prev->next(si->next());
            if (si == sonext) {
                sonext = prev;
            }

            LOGD("removed soinfo: %s", si->get_realpath());

            return true;
        }

        const auto initialized = []() {
            SandHook::ElfImg linker(GetLinkerPath());
            return ProtectedDataGuard::setup(linker) &&
                   (solist = *reinterpret_cast<soinfo **>(linker.getSymbAddress(
                           "__dl__ZL6solist"))) != nullptr &&
                   (sonext = *reinterpret_cast<soinfo **>(linker.getSymbAddress(
                           "__dl__ZL6sonext"))) != nullptr &&
                   (soinfo::get_realpath_sym = reinterpret_cast<decltype(soinfo::get_realpath_sym)>(linker.getSymbAddress(
                           "__dl__ZNK6soinfo12get_realpathEv"))) != nullptr;
        }();

        std::vector<soinfo *> linker_get_solist() {
            std::vector<soinfo *> linker_solist{};
            for(auto *iter = solist; iter; iter = iter->next()) {
                linker_solist.push_back(iter);
            }
            return linker_solist;
        }

        void RemovePathsFromSolist(const std::vector<const char *> &names) {
            if (!initialized) {
                LOGW("not initialized");
                return;
            }
            ProtectedDataGuard g;
            auto list = linker_get_solist();
            for (const auto &soinfo : linker_get_solist()) {
                const char *real_path = soinfo->get_realpath();
                if (real_path != nullptr) {
                    for (const auto &path : names) {
                        if (strcmp(path, real_path) == 0) {
                            solist_remove_soinfo(soinfo);
                            break;
                        }
                    }
                }
            }
        }

        using riru_hide_t = int(const char *const *names, int names_count);

        void *riru_hide_handle;
        riru_hide_t *riru_hide_func;

        void HidePathsFromMaps(const std::vector<const char *> &names) {
            if (!riru_hide_func) return;

            LOGD("do hide");
            riru_hide_func(&names[0], names.size());

            // cleanup riruhide.so
            LOGD("dlclose");
            if (dlclose(riru_hide_handle) != 0) {
                LOGE("dlclose failed: %s", dlerror());
                return;
            }
        }
    }

    void HideFromMaps() {
        auto self_path = Magisk::GetPathForSelfLib("libriru.so");
        auto modules = Modules::Get();
        std::vector<const char *> names{};
        for (auto module : Modules::Get()) {
            if (strcmp(module->id, MODULE_NAME_CORE) == 0) {
                names.push_back(self_path.c_str());
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
        if (!names.empty()) Hide::HidePathsFromMaps(names);
    }

    static void RemoveFromSoList(const std::vector<const char *> &names) {
        Hide::RemovePathsFromSolist(names);
    }

    static void HideFromSoList(const std::vector<const char *> &names) {
        auto callback = [](struct dl_phdr_info *info, size_t size, void *data) {
            auto names = *((std::vector<const char *> *) data);

            for (const auto &path : names) {
                if (!info->dlpi_name) continue;
                if (strcmp(path, info->dlpi_name) == 0) {
                    memset((void *) info->dlpi_name, 0, strlen(path));
                    LOGD("hide %s from dl_iterate_phdr", path);
                    break;
                }
            }
            return 0;
        };
        dl_iterate_phdr(callback, (void *) &names);
    }

    void HideFromSoList() {
        auto self_path = Magisk::GetPathForSelfLib("libriru.so");
        auto modules = Modules::Get();
        std::vector<const char *> names_to_remove{};
        std::vector<const char *> names_to_wipe{};
        for (auto module : Modules::Get()) {
            if (strcmp(module->id, MODULE_NAME_CORE) == 0) {
                if (Entry::IsSelfUnloadAllowed()) {
                    LOGD("don't hide self since it will be unloaded");
                } else {
                    names_to_remove.push_back(self_path.c_str());
                }
                names_to_wipe.push_back(self_path.c_str());
            } else if (module->supportHide) {
                if (!module->isLoaded()) {
                    LOGD("%s is unloaded", module->id);
                    continue;
                }
                if (module->apiVersion <= 24) {
                    LOGW("%s is too old to hide so", module->id);
                } else {
                    names_to_remove.push_back(module->path);
                }
                names_to_wipe.push_back(module->path);
            } else {
                LOGD("module %s does not support hide", module->id);
                names_to_wipe.push_back(module->path);
            }
        }

        if (AndroidProp::GetApiLevel() >= 23 && !names_to_remove.empty()) {
            RemoveFromSoList(names_to_remove);
        }

        if (!names_to_wipe.empty()) {
            HideFromSoList(names_to_wipe);
        }
    }

    void PrepareMapsHideLibrary() {
        auto hide_lib_path = Magisk::GetPathForSelfLib("libriruhide.so");

        // load riruhide.so and run the hide
        LOGD("dlopen libriruhide");
        riru_hide_handle = dlopen_ext(hide_lib_path.c_str(), 0);
        if (!riru_hide_handle) {
            LOGE("dlopen %s failed: %s", hide_lib_path.c_str(), dlerror());
            return;
        }
        riru_hide_func = (riru_hide_t *) dlsym(riru_hide_handle, "riru_hide");
        if (!riru_hide_func) {
            LOGE("dlsym failed: %s", dlerror());
            dlclose(riru_hide_handle);
            return;
        }
    }
}
