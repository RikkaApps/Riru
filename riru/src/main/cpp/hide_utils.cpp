#include <sys/mman.h>
#include <dlfcn.h>
#include <dl.h>
#include <link.h>
#include <string>
#include <magisk.h>
#include <android_prop.h>
#include "hide_utils.h"
#include "logging.h"
#include "module.h"
#include "entry.h"
#include <iostream>
#include <elf_util.h>
#include <set>

namespace hide {
    namespace {
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

        struct soinfo;

        soinfo *solist = nullptr;
        soinfo **sonext = nullptr;
        soinfo *somain = nullptr;

        template<typename T>
        inline T *getStaticVariable(const SandHook::ElfImg &linker, std::string_view name) {
            auto *addr = reinterpret_cast<T **>(linker.getSymbAddress(name.data()));
            return addr == nullptr ? nullptr : *addr;
        }

        struct soinfo {
            soinfo *next() {
                return *(soinfo **) ((uintptr_t) this + solist_next_offset);
            }

            void next(soinfo *si) {
                *(soinfo **) ((uintptr_t) this + solist_next_offset) = si;
            }

            const char *get_realpath() {
                return get_realpath_sym ? get_realpath_sym(this) : ((std::string *) (
                        (uintptr_t) this + solist_realpath_offset))->c_str();

            }

            static bool setup(const SandHook::ElfImg &linker) {
                get_realpath_sym = reinterpret_cast<decltype(get_realpath_sym)>(linker.getSymbAddress(
                        "__dl__ZNK6soinfo12get_realpathEv"));
                auto vsdo = getStaticVariable<soinfo>(linker, "__dl__ZL4vdso");
                for (size_t i = 0; i < 1024 / sizeof(void *); i++) {
                    auto *possible_next = *(void **) ((uintptr_t) solist + i * sizeof(void *));
                    if (possible_next == somain || (vsdo != nullptr && possible_next == vsdo)) {
                        solist_next_offset = i * sizeof(void *);
                        return android_prop::GetApiLevel() < 26 || get_realpath_sym != nullptr;
                    }
                }
                LOGW("failed to search next offset");
                // shortcut
                return android_prop::GetApiLevel() < 26 || get_realpath_sym != nullptr;
            }

#ifdef __LP64__
            constexpr static size_t solist_realpath_offset = 0x1a8;
            inline static size_t solist_next_offset = 0x30;
#else
            constexpr static size_t solist_realpath_offset = 0x174;
            inline static size_t solist_next_offset = 0xa4;
#endif

            // since Android 8
            inline static const char *(*get_realpath_sym)(soinfo *);
        };

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
            if (si == *sonext) {
                *sonext = prev;
            }

            LOGD("removed soinfo: %s", si->get_realpath());

            return true;
        }

        const auto initialized = []() {
            SandHook::ElfImg linker("/linker");
            return ProtectedDataGuard::setup(linker) &&
                   (solist = getStaticVariable<soinfo>(linker, "__dl__ZL6solist")) != nullptr &&
                   (sonext = linker.getSymbAddress<soinfo**>("__dl__ZL6sonext")) != nullptr &&
                   (somain = getStaticVariable<soinfo>(linker, "__dl__ZL6somain")) != nullptr &&
                   soinfo::setup(linker);
        }();

        std::list<soinfo *> linker_get_solist() {
            std::list<soinfo *> linker_solist{};
            for (auto *iter = solist; iter; iter = iter->next()) {
                linker_solist.push_back(iter);
            }
            return linker_solist;
        }

        void RemovePathsFromSolist(const std::set<std::string_view> &names) {
            if (!initialized) {
                LOGW("not initialized");
                return;
            }
            ProtectedDataGuard g;
            for (const auto &soinfo : linker_get_solist()) {
                const auto &real_path = soinfo->get_realpath();
                if (real_path && names.count(real_path)) {
                    solist_remove_soinfo(soinfo);
                }
            }
        }

        using riru_hide_t = int(const std::set<std::string_view> &names);

        void *riru_hide_handle;
        riru_hide_t *riru_hide_func;

        void HidePathsFromMaps(const std::set<std::string_view> &names) {
            if (!riru_hide_func) return;

            LOGD("do hide");
            riru_hide_func(names);

            // cleanup riruhide.so
            LOGD("dlclose");
            if (dlclose(riru_hide_handle) != 0) {
                LOGE("dlclose failed: %s", dlerror());
                return;
            }
        }
    }  // namespace

    void HideFromMaps() {
        auto self_path = magisk::GetPathForSelfLib("libriru.so");
        std::set<std::string_view> names{self_path};
        for (const auto &module : modules::Get()) {
            if (module.supportHide) {
                if (!module.isLoaded()) {
                    LOGD("%s is unloaded", module.id.data());
                } else {
                    names.emplace(module.path);
                }
            } else {
                LOGD("module %s does not support hide", module.id.data());
            }
        }
        if (!names.empty()) hide::HidePathsFromMaps(names);
    }

    static void RemoveFromSoList(const std::set<std::string_view> &names) {
        hide::RemovePathsFromSolist(names);
    }

    void HideFromSoList() {
        auto self_path = magisk::GetPathForSelfLib("libriru.so");
        std::set<std::string_view> names_to_remove{};
        if (Entry::IsSelfUnloadAllowed()) {
            LOGD("don't hide self since it will be unloaded");
        } else {
            names_to_remove.emplace(self_path);
        }
        for (const auto &module : modules::Get()) {
            if (module.supportHide) {
                if (!module.isLoaded()) {
                    LOGD("%s is unloaded", module.id.data());
                    continue;
                }
                if (module.apiVersion < 24) {
                    LOGW("%s is too old to hide so", module.id.data());
                } else {
                    names_to_remove.emplace(module.path);
                }
            } else {
                LOGD("module %s does not support hide", module.id.data());
            }
        }

        if (android_prop::GetApiLevel() >= 23 && !names_to_remove.empty()) {
            RemoveFromSoList(names_to_remove);
        }
    }

    void PrepareMapsHideLibrary() {
        auto hide_lib_path = magisk::GetPathForSelfLib("libriruhide.so");

        // load riruhide.so and run the hide
        LOGD("dlopen libriruhide");
        riru_hide_handle = DlopenExt(hide_lib_path.c_str(), 0);
        if (!riru_hide_handle) {
            LOGE("dlopen %s failed: %s", hide_lib_path.c_str(), dlerror());
            return;
        }
        riru_hide_func = reinterpret_cast<riru_hide_t *>(dlsym(riru_hide_handle, "riru_hide"));
        if (!riru_hide_func) {
            LOGE("dlsym failed: %s", dlerror());
            dlclose(riru_hide_handle);
            return;
        }
    }
}  // namespace Hide
