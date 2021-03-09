#include <cstddef>
#include <plt.h>
#include <dlfcn.h>
#include <libgen.h>
#include <cstring>
#include <logging.h>

#define FUNC_DEF(ret, func, ...) \
    using func##_t = ret(__VA_ARGS__); \
    static func##_t *func;

FUNC_DEF(void, android_get_LD_LIBRARY_PATH, char *buffer, size_t buffer_size);
FUNC_DEF(void, android_update_LD_LIBRARY_PATH, const char *ld_library_path);


void *dl_dlopen(const char *path, int flags) {
    auto h = dlopen(path, flags);
    if (h) return h;

    if (!android_get_LD_LIBRARY_PATH)
        android_get_LD_LIBRARY_PATH = (android_get_LD_LIBRARY_PATH_t *) plt_dlsym("android_get_LD_LIBRARY_PATH", nullptr);

    if (!android_update_LD_LIBRARY_PATH)
        android_update_LD_LIBRARY_PATH = (android_update_LD_LIBRARY_PATH_t *) plt_dlsym("android_update_LD_LIBRARY_PATH", nullptr);

    if (!android_get_LD_LIBRARY_PATH || !android_update_LD_LIBRARY_PATH) return h;

    char ld_path[4096];
    android_get_LD_LIBRARY_PATH(ld_path, sizeof(ld_path));
    LOGD("LD_LIBRARY_PATH is %s", ld_path);

    auto len = strlen(ld_path);
    ld_path[len] = ':';

    auto dir = dirname(path);
    strcpy(ld_path + len + 1, dir);
    LOGD("new LD_LIBRARY_PATH is %s", ld_path);
    android_update_LD_LIBRARY_PATH(ld_path);

    h = dlopen(path, flags);

    ld_path[len] = '\0';
    LOGD("restore LD_LIBRARY_PATH to %s", ld_path);
    android_update_LD_LIBRARY_PATH(ld_path);

    return h;
}