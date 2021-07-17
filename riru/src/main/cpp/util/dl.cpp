#include <cstddef>
#include <dlfcn.h>
#include <libgen.h>
#include <cstring>
#include <logging.h>
#include <android/dlext.h>
#include <android_prop.h>


extern "C" [[gnu::weak]]struct android_namespace_t *
//NOLINTNEXTLINE
android_create_namespace([[maybe_unused]] const char *name,
                         [[maybe_unused]] const char *ld_library_path,
                         [[maybe_unused]] const char *default_library_path,
                         [[maybe_unused]] uint64_t type,
                         [[maybe_unused]] const char *permitted_when_isolated_path,
                         [[maybe_unused]] android_namespace_t *parent) {
    return nullptr;
}

void *DlopenExt(const char *path, int flags) {
    auto info = android_dlextinfo{};

    if (AndroidProp::GetApiLevel() >= __ANDROID_API_Q__) {
//        FIND_FUNC(android_create_namespace)

//        if (android_create_namespace) {
        auto *dir = dirname(path);
        auto *ns = android_create_namespace(path, dir, nullptr, 2/*ANDROID_NAMESPACE_TYPE_SHARED*/,
                                           nullptr, nullptr);
        if (ns) {
            info.flags = ANDROID_DLEXT_USE_NAMESPACE;
            info.library_namespace = ns;

            LOGD("open %s with namespace %p", path, ns);
        } else {
            LOGD("cannot create namespace for %s", path);
        }
    }

    auto *handle = android_dlopen_ext(path, flags, &info);
    if (handle) {
        LOGD("dlopen %s: %p", path, handle);
    } else {
        LOGE("dlopen %s: %s", path, dlerror());
    }
    return handle;
}
