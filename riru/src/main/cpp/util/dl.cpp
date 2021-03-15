#include <cstddef>
#include <plt.h>
#include <dlfcn.h>
#include <libgen.h>
#include <cstring>
#include <logging.h>
#include <android/dlext.h>
#include <android_prop.h>

#define FUNC_DEF(ret, func, ...) \
    using func##_t = ret(__VA_ARGS__); \
    static func##_t *func;

#define FIND_FUNC(func) \
    if (!func) \
    func = (func##_t *) plt_dlsym(#func, nullptr);

FUNC_DEF(void, android_get_LD_LIBRARY_PATH, char *buffer, size_t buffer_size)
FUNC_DEF(void, android_update_LD_LIBRARY_PATH, const char *ld_library_path)

FUNC_DEF(android_namespace_t*, android_create_namespace,
         const char *name,
         const char *ld_library_path,
         const char *default_library_path,
         uint64_t type,
         const char *permitted_when_isolated_path,
         android_namespace_t *parent)

void *dlopen_ext(const char *path, int flags) {
    auto info = android_dlextinfo{};

    if (AndroidProp::GetApiLevel() >= 28) {
        FIND_FUNC(android_create_namespace)

        if (android_create_namespace) {
            auto dir = dirname(path);
            auto ns = android_create_namespace(path, dir, nullptr, 2/*ANDROID_NAMESPACE_TYPE_SHARED*/, nullptr, nullptr);
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
        LOGD("dloepn %s: %p", path, handle);
    } else {
        LOGE("dlopen %s: %s", path, dlerror());
    }
    return handle;
}