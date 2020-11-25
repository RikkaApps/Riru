#include <unistd.h>
#include <dlfcn.h>

static int stub(const char *) {
    return 0;
}

int (*setsockcreatecon)(const char *con) = stub;

void dload_selinux() {
    if (access("/system/lib/libselinux.so", F_OK) != 0) {
        return;
    }


#ifdef __LP64__
    void *handle = dlopen("/system/lib64/libselinux.so", RTLD_LAZY | RTLD_LOCAL);
#else
    void *handle = dlopen("/system/lib/libselinux.so", RTLD_LAZY | RTLD_LOCAL);
#endif

    if (!handle) return;

    setsockcreatecon = (int (*)(const char *)) dlsym(handle, "setsockcreatecon");
}