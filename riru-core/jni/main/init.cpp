#include <dlfcn.h>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <jni.h>
#include <cstring>
#include <cstdlib>
#include <sys/mman.h>
#include <array>
#include <thread>
#include <vector>
#include <utility>
#include <elf.h>
#include <string>
#include <dirent.h>
#include <xhook/xhook.h>

#include "misc.h"
#include "jni_native_method.h"
#include "logging.h"
#include "wrap.h"
#include "module.h"
#include "JNIHelper.h"

#define CONFIG_DIR "/data/misc/riru"

#ifdef __LP64__
#define ZYGOTE_NAME "zygote64"
#define APP_PROCESS_NAME "/system/bin/app_process64"
#define ANDROID_RUNTIME_LIBRARY "/system/lib64/libandroid_runtime.so"
#else
#define ZYGOTE_NAME "zygote"
#define APP_PROCESS_NAME "/system/bin/app_process"
#define ANDROID_RUNTIME_LIBRARY "/system/lib/libandroid_runtime.so"
#endif

#define NATIVE_FORK_AND_SPECIALIZE_METHOD "nativeForkAndSpecialize"
#define NATIVE_FORK_SYSTEM_SERVER_METHOD "nativeForkSystemServer"

void init(JNIEnv *jniEnv);

std::vector<module *> *get_modules() {
    static std::vector<module *> *modules = new std::vector<module *>();
    return modules;
}

#ifdef __LP64__
#define MODULE_PATH_FMT "/system/lib64/libriru_%s.so"
#else
#define MODULE_PATH_FMT "/system/lib/libriru_%s.so"
#endif

void load_modules() {
    DIR *dir;
    struct dirent *entry;
    char path[256];
    void *handle;

    if (!(dir = _opendir(CONFIG_DIR "/modules")))
        return;

    while ((entry = _readdir(dir))) {
        if (entry->d_type == DT_DIR) {
            if (entry->d_name[0] == '.')
                continue;

            snprintf(path, 256, MODULE_PATH_FMT, entry->d_name);

            if (access(path, F_OK) != 0) {
                PLOGE("access %s", path);
                continue;
            }

            handle = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
            if (!handle) {
                PLOGE("dlopen %s", path);
                continue;
            }

            module *module = new struct module();
            module->handle = handle;
            module->name = strdup(entry->d_name);
            module->onModuleLoaded = dlsym(handle, "onModuleLoaded");
            module->forkAndSpecializePre = dlsym(handle, "nativeForkAndSpecializePre");
            module->forkAndSpecializePost = dlsym(handle, "nativeForkAndSpecializePost");
            module->forkSystemServerPre = dlsym(handle, "nativeForkSystemServerPre");
            module->forkSystemServerPost = dlsym(handle, "nativeForkSystemServerPost");

            get_modules()->push_back(module);

#ifdef __LP64__
            LOGI("module loaded: %s %lu", module->name, get_modules()->size());
#else
            LOGI("module loaded: %s %u", module->name, get_modules()->size());
#endif

            if (module->onModuleLoaded) {
                LOGV("%s: onModuleLoaded", module->name);

                ((loaded_t) module->onModuleLoaded)();
            }
        }
    }

    closedir(dir);
}

#define XHOOK_REGISTER(NAME) \
    if (xhook_register(".*", #NAME, (void*) new_##NAME, (void **) &old_##NAME) != 0) \
        LOGE("failed to register hook " #NAME "."); \

#define NEW_FUNC_DEF(ret, func, ...) \
    static ret (*old_##func)(__VA_ARGS__); \
    static ret new_##func(__VA_ARGS__)

NEW_FUNC_DEF(int, _ZN7android39register_com_android_internal_os_ZygoteEP7_JNIEnv, JNIEnv *env) {
    LOGI("register_com_android_internal_os_Zygote");
    int res = old__ZN7android39register_com_android_internal_os_ZygoteEP7_JNIEnv(env);
    if (res >= 0)
        init(env);
    return res;
}

extern "C" void con() __attribute__((constructor));

void con() {
    static int loaded = 0;
    if (loaded)
        return;

    loaded = 1;

    if (access(CONFIG_DIR "/.disable", F_OK) == 0) {
        LOGI(CONFIG_DIR "/.disable exists, do nothing.");
        return;
    }

    char buf[64];
    get_proc_name(getpid(), buf, 63);
    if (strncmp(ZYGOTE_NAME, buf, strlen(ZYGOTE_NAME)) != 0 &&
        strncmp(APP_PROCESS_NAME, buf, strlen(APP_PROCESS_NAME)) != 0) {
        return;
    }

    load_modules();

    XHOOK_REGISTER(_ZN7android39register_com_android_internal_os_ZygoteEP7_JNIEnv);

    if (xhook_refresh(0) == 0) {
        xhook_clear();
        LOGI("hook installed");
    } else {
        LOGE("failed to refresh hook");
    }
}

static JNINativeMethod gMethods[] = {{NULL, NULL, NULL},
                                     {NULL, NULL, NULL}};

void *_nativeForkAndSpecialize = NULL;
void *_nativeForkSystemServer = NULL;

JNINativeMethod *search_method(int endian, std::vector<std::pair<uintptr_t, uintptr_t>> addresses,
                               const char *name, size_t len) {
    // Step 1: search for name
    uintptr_t str_addr = 0;
    for (auto address : addresses) {
        void *res = memsearch(address.first, address.second, (const void *) name, len);
        if (res) {
            str_addr = (uintptr_t) res;
#ifdef __LP64__
            LOGI("found \"%s\" at 0x%lx", (char *) str_addr, str_addr);
#else
            LOGI("found \"%s\" at 0x%x", (char *) str_addr, str_addr);
#endif
            break;
        }
    }

    if (!str_addr) {
        LOGE("\" %s \" not found.", name);
        return NULL;
    }

    // Step 2: search JNINativeMethod with str_addr's address
    size_t size = sizeof(uintptr_t);
    unsigned char *data = new unsigned char[size];

    for (size_t i = 0; i < size; i++)
        data[endian == ELFDATA2LSB ? i : size - i - 1] = (unsigned char) (
                ((uintptr_t) 0xff << i * 8 & str_addr) >> i * 8);

    JNINativeMethod *method = NULL;
    for (auto address : addresses) {
        void *res = memsearch(address.first, address.second, data, size);
        if (res) {
            method = (JNINativeMethod *) res;
#ifdef __LP64__
            LOGI("found {\"%s\", \"%s\", %p} at 0x%lx", method->name, method->signature,
                 method->fnPtr, (uintptr_t) method);
#else
            LOGI("found {\"%s\", \"%s\", %p} at 0x%x", method->name, method->signature,
                 method->fnPtr, (uintptr_t) method);
#endif
            break;
        }
    }
    if (!method) {
        LOGE("%s not found.", name);
        return NULL;
    }
    return method;
}

void init(JNIEnv *jniEnv) {
    // Step 0: read elf header for endian
    int endian;

    FILE *file = fopen(ANDROID_RUNTIME_LIBRARY, "r");
    if (!file) {
        LOGE("fopen " ANDROID_RUNTIME_LIBRARY " failed.");
        return;
    }

#ifdef __LP64__
    Elf64_Ehdr header;
#else
    Elf32_Ehdr header;
#endif
    fread(&header, 1, sizeof(header), file);
    endian = header.e_ident[EI_DATA];

    fclose(file);

    // Step 1: get libandroid_runtime.so address
    std::vector<std::pair<uintptr_t, uintptr_t>> addresses;

    int fd = open("/proc/self/maps", O_RDONLY);
    if (fd == -1) {
        LOGE("open /proc/self/maps failed.");
        return;
    }

#if __LP64__
    const char *s = "%lx-%lx %s %*s %*s %*s %s";
#else
    const char *s = "%x-%x %s %*s %*s %*s %s";
#endif

    char buf[512];
    while (fdgets(buf, 512, fd) > 0) {
        uintptr_t start = 0, end = 0;
        char flags[5], filename[128];
        if (sscanf(buf, s, &start, &end, flags, filename) != 4)
            continue;

        if (strcmp(ANDROID_RUNTIME_LIBRARY, filename) == 0) {
            addresses.push_back(std::pair<uintptr_t, uintptr_t>(start, end));
            LOGD("%lx %lx %s %s", start, end, flags, filename);
        }
    }
    close(fd);

    JNINativeMethod *method[2];
    method[0] = search_method(endian, addresses, NATIVE_FORK_AND_SPECIALIZE_METHOD,
                              strlen(NATIVE_FORK_AND_SPECIALIZE_METHOD) + 1);
    method[1] = search_method(endian, addresses, NATIVE_FORK_SYSTEM_SERVER_METHOD,
                              strlen(NATIVE_FORK_SYSTEM_SERVER_METHOD) + 1);

    if (!method[0] || !method[1]) {
        LOGE("JNINativeMethod not found.");
        return;
    }

    // Step 2: make our JNINativeMethod
    _nativeForkAndSpecialize = method[0]->fnPtr;
    _nativeForkSystemServer = method[1]->fnPtr;

    // nativeForkAndSpecialize
    if (strncmp(nativeForkAndSpecialize_marshmallow_sig, method[0]->signature,
                strlen(nativeForkAndSpecialize_marshmallow_sig)) == 0)
        gMethods[0].fnPtr = (void *) nativeForkAndSpecialize_marshmallow;
    else if (strncmp(nativeForkAndSpecialize_oreo_sig, method[0]->signature,
                     strlen(nativeForkAndSpecialize_oreo_sig)) == 0)
        gMethods[0].fnPtr = (void *) nativeForkAndSpecialize_oreo;
    else if (strncmp(nativeForkAndSpecialize_p_sig, method[0]->signature,
                     strlen(nativeForkAndSpecialize_p_sig)) == 0)
        gMethods[0].fnPtr = (void *) nativeForkAndSpecialize_p;

    // nativeForkSystemServer
    if (strncmp(nativeForkSystemServer_sig, method[1]->signature,
                strlen(nativeForkSystemServer_sig)) == 0)
        gMethods[1].fnPtr = (void *) nativeForkSystemServer;

    if (!gMethods[0].fnPtr || !gMethods[1].fnPtr) {
        LOGE("not matching method found.");
        return;
    }

    gMethods[0].name = method[0]->name;
    gMethods[1].name = method[1]->name;
    gMethods[0].signature = method[0]->signature;
    gMethods[1].signature = method[1]->signature;

    // Step 3: replace native method with RegisterNatives
    jclass clazz = JNI_FindClass(jniEnv, "com/android/internal/os/Zygote");
    if (!clazz) {
        LOGE("class com/android/internal/os/Zygote not found");
        return;
    }

    jint res = JNI_RegisterNatives(jniEnv, clazz, gMethods, 2);

    if (res != JNI_OK) {
        LOGE("RegisterNatives failed");
        return;
    } else {
        LOGI("replaced com.android.internal.os.Zygote#nativeForkAndSpecialize & com.android.internal.os.Zygote#nativeForkSystemServer");
    }
}