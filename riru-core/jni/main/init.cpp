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
#include <map>
#include <utility>
#include <string>
#include <dirent.h>
#include <xhook/xhook.h>
#include <iterator>

#include "misc.h"
#include "jni_native_method.h"
#include "logging.h"
#include "wrap.h"
#include "module.h"
#include "JNIHelper.h"
#include "api.h"

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

#ifdef __LP64__
#define MODULE_PATH_FMT "/system/lib64/libriru_%s.so"
#else
#define MODULE_PATH_FMT "/system/lib/libriru_%s.so"
#endif

static void load_modules() {
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

            handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
            if (!handle) {
                PLOGE("dlopen %s", path);
                continue;
            }

            auto *module = new struct module();
            module->handle = handle;
            module->name = strdup(entry->d_name);
            module->onModuleLoaded = dlsym(handle, "onModuleLoaded");
            module->forkAndSpecializePre = dlsym(handle, "nativeForkAndSpecializePre");
            module->forkAndSpecializePost = dlsym(handle, "nativeForkAndSpecializePost");
            module->forkSystemServerPre = dlsym(handle, "nativeForkSystemServerPre");
            module->forkSystemServerPost = dlsym(handle, "nativeForkSystemServerPost");
            module->funcs = new std::map<std::string, void *>();

            get_modules()->push_back(module);

            void *sym = dlsym(handle, "riru_set_module_name");
            if (sym)
                ((void (*)(const char*)) sym)(module->name);

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


static void onRegisterZygote(JNIEnv *env, const char *className, const JNINativeMethod *methods,
                      int numMethods) {
    JNINativeMethod zygoteMethods[] = {{nullptr, nullptr, nullptr},
                                       {nullptr, nullptr, nullptr}};
    JNINativeMethod method;
    for (int i = 0; i < numMethods; ++i) {
        method = methods[i];

        if (strcmp(method.name, "nativeForkAndSpecialize") == 0) {
            set_nativeForkAndSpecialize(method.fnPtr);

            zygoteMethods[0].name = method.name;
            zygoteMethods[0].signature = method.signature;
            zygoteMethods[0].fnPtr = nullptr;

            if (strcmp(nativeForkAndSpecialize_marshmallow_sig, method.signature) == 0)
                zygoteMethods[0].fnPtr = (void *) nativeForkAndSpecialize_marshmallow;
            else if (strcmp(nativeForkAndSpecialize_oreo_sig, method.signature) == 0)
                zygoteMethods[0].fnPtr = (void *) nativeForkAndSpecialize_oreo;
            else if (strcmp(nativeForkAndSpecialize_p_sig, method.signature) == 0)
                zygoteMethods[0].fnPtr = (void *) nativeForkAndSpecialize_p;
            else if (strcmp(nativeForkAndSpecialize_samsung_o_sig, method.signature) == 0)
                zygoteMethods[0].fnPtr = (void *) nativeForkAndSpecialize_samsung_o;
            else
                LOGW("found nativeForkAndSpecialize but signature %s mismatch", method.signature);
        } else if (strcmp(method.name, "nativeForkSystemServer") == 0) {
            set_nativeForkSystemServer(method.fnPtr);

            zygoteMethods[1].name = method.name;
            zygoteMethods[1].signature = method.signature;
            zygoteMethods[1].fnPtr = nullptr;

            if (strcmp(nativeForkSystemServer_sig, method.signature) == 0)
                zygoteMethods[1].fnPtr = (void *) nativeForkSystemServer;
            else
                LOGW("found nativeForkSystemServer but signature %s mismatch", method.signature);
        }
    }

    LOGI("{\"%s\", \"%s\", %p}", zygoteMethods[0].name, zygoteMethods[0].signature,
         zygoteMethods[0].fnPtr);
    LOGI("{\"%s\", \"%s\", %p}", zygoteMethods[1].name, zygoteMethods[1].signature,
         zygoteMethods[1].fnPtr);

    if (zygoteMethods[0].fnPtr && zygoteMethods[1].fnPtr) {
        jclass clazz = JNI_FindClass(env, className);
        if (clazz) {
            jint r = JNI_RegisterNatives(env, clazz, zygoteMethods, 2);
            if (r != JNI_OK) {
                LOGE("RegisterNatives failed");
            } else {
                LOGI("replaced com.android.internal.os.Zygote#nativeForkAndSpecialize & com.android.internal.os.Zygote#nativeForkSystemServer");
            }
        } else {
            LOGE("class com/android/internal/os/Zygote not found");
        }
    }
}

static void onRegisterSystemProperties(JNIEnv *env, const char *className, const JNINativeMethod *methods,
                                int numMethods) {
    JNINativeMethod systemPropertiesMethods[] = {{nullptr, nullptr, nullptr}};
    JNINativeMethod method;
    for (int i = 0; i < numMethods; ++i) {
        method = methods[i];

        if (strcmp(method.name, "native_set") == 0) {
            set_SystemProperties_set(method.fnPtr);

            systemPropertiesMethods[0].name = method.name;
            systemPropertiesMethods[0].signature = method.signature;

            if (strcmp("(Ljava/lang/String;Ljava/lang/String;)V", method.signature) == 0)
                systemPropertiesMethods[0].fnPtr = (void *) SystemProperties_set;
        }
    }

    LOGI("{\"%s\", \"%s\", %p}", systemPropertiesMethods[0].name,
         systemPropertiesMethods[0].signature,
         systemPropertiesMethods[0].fnPtr);

    if (systemPropertiesMethods[0].fnPtr) {
        jclass clazz = JNI_FindClass(env, className);
        if (clazz) {
            jint r = JNI_RegisterNatives(env, clazz, systemPropertiesMethods, 1);
            if (r != JNI_OK) {
                LOGE("RegisterNatives failed");
            } else {
                LOGI("replaced android.os.SystemProperties#native_set");
            }
        } else {
            LOGE("class android/os/SystemProperties not found");
        }
    }
}

#define XHOOK_REGISTER(NAME) \
    if (xhook_register(".*", #NAME, (void*) new_##NAME, (void **) &old_##NAME) != 0) \
        LOGE("failed to register hook " #NAME "."); \

#define NEW_FUNC_DEF(ret, func, ...) \
    static ret (*old_##func)(__VA_ARGS__); \
    static ret new_##func(__VA_ARGS__)

NEW_FUNC_DEF(int, jniRegisterNativeMethods, JNIEnv *env, const char *className,
             const JNINativeMethod *methods, int numMethods) {
    put_native_method(className, methods, numMethods);

    LOGV("jniRegisterNativeMethods %s", className);

    int res = old_jniRegisterNativeMethods(env, className, methods, numMethods);

    if (strcmp("com/android/internal/os/Zygote", className) == 0) {
        onRegisterZygote(env, className, methods, numMethods);
    } else if (strcmp("android/os/SystemProperties", className) == 0) {
        // hook android.os.SystemProperties#native_set to prevent a critical problem on Android 9+
        // see comment of SystemProperties_set in jni_native_method.cpp for detail
        onRegisterSystemProperties(env, className, methods, numMethods);
    }

    return res;
}

extern "C" void riru_constructor() __attribute__((constructor));

void riru_constructor() {
    static int loaded = 0;
    if (loaded)
        return;

    loaded = 1;

    if (access(CONFIG_DIR "/.disable", F_OK) == 0) {
        LOGI(CONFIG_DIR
                     "/.disable exists, do nothing.");
        return;
    }

    char buf[64];
    get_proc_name(getpid(), buf, 63);
    if (strncmp(ZYGOTE_NAME, buf, strlen(ZYGOTE_NAME)) != 0 &&
        strncmp(APP_PROCESS_NAME, buf, strlen(APP_PROCESS_NAME)) != 0) {
        return;
    }

    XHOOK_REGISTER(jniRegisterNativeMethods);

    if (xhook_refresh(0) == 0) {
        xhook_clear();
        LOGI("hook installed");
    } else {
        LOGE("failed to refresh hook");
    }

    load_modules();
}