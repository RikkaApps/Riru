#include <cstddef>
#include <dlfcn.h>
#include <libgen.h>
#include <cstring>
#include <logging.h>
#include <android/dlext.h>
#include <android_prop.h>
#include "elf_util.h"

void *dlopen_ext(const char *path, int flags) {
    auto info = android_dlextinfo{};
    if (AndroidProp::GetApiLevel() >= 28) {
        static auto android_create_namespace = reinterpret_cast<android_namespace_t *(*)(
                const char *, const char *, const char *, uint64_t, const char *,
                android_namespace_t *, const void *)>(SandHook::ElfImg(LINKER_PATH)
                .getSymbAddress("__loader_android_create_namespace"));

        LOGD("create namespace %p", android_create_namespace);
        if (android_create_namespace) {
            auto dir = dirname(path);
            auto ns = android_create_namespace(path, dir, nullptr,
                                               2/*ANDROID_NAMESPACE_TYPE_SHARED*/, nullptr,
                                               nullptr,
                                               reinterpret_cast<const void *>(&dlopen_ext));
            if (ns) {
                info.flags = ANDROID_DLEXT_USE_NAMESPACE;
                info.library_namespace = ns;

                LOGD("open %s with namespace %p", path, ns);
            } else {
                LOGD("cannot create namespace for %s", path);
            }
        } else {
            LOGD("cannot find android_create_namespace");
        }
    }

    auto handle = android_dlopen_ext(path, flags, &info);
    if (handle) {
        LOGD("dlopen %s: %p", path, handle);
    } else {
        LOGE("dlopen %s: %s", path, dlerror());
    }
    return handle;
}
