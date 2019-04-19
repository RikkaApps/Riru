#include <dlfcn.h>
#include <cstdio>
#include <unistd.h>
#include <jni.h>
#include <cstring>
#include <cstdlib>
#include <array>
#include <thread>
#include <vector>
#include <map>
#include <utility>
#include <string>
#include <dirent.h>
#include <xhook/xhook.h>
#include <iterator>
#include <sys/system_properties.h>

#include "misc.h"
#include "jni_native_method.h"
#include "logging.h"
#include "wrap.h"
#include "module.h"
#include "JNIHelper.h"
#include "api.h"

#define CONFIG_DIR "/data/misc/riru"

#ifdef __LP64__
#define MODULE_PATH_FMT "/system/lib64/libriru_%s.so"
#else
#define MODULE_PATH_FMT "/system/lib/libriru_%s.so"
#endif

static int methods_replaced = 0;
static int sdkLevel;
static char androidVersionName[PROP_VALUE_MAX + 1];

int riru_is_zygote_methods_replaced() {
    return methods_replaced;
}

static int isQ() {
    return strcmp("Q", androidVersionName) == 0 || sdkLevel >= 29;
}

static void load_modules() {
    DIR *dir;
    struct dirent *entry;
    char path[PATH_MAX], module_prop[PATH_MAX], api[PATH_MAX];
    int moduleApiVersion;
    void *handle;

    if (!(dir = _opendir(CONFIG_DIR "/modules")))
        return;

    while ((entry = _readdir(dir))) {
        if (entry->d_type == DT_DIR) {
            if (entry->d_name[0] == '.')
                continue;

            snprintf(path, PATH_MAX, MODULE_PATH_FMT, entry->d_name);

            if (access(path, F_OK) != 0) {
                PLOGE("access %s", path);
                continue;
            }

            snprintf(module_prop, PATH_MAX, CONFIG_DIR "/modules/%s/module.prop", entry->d_name);
            if (access(module_prop, F_OK) != 0) {
                PLOGE("access %s", module_prop);
                continue;
            }

            moduleApiVersion = -1;
            if (get_prop(module_prop, "api", api) > 0) {
                moduleApiVersion = atoi(api);
            }

            if (isQ() && moduleApiVersion < 3) {
                LOGW("module %s does not support Android Q", entry->d_name);
                continue;
            }

            handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
            if (!handle) {
                PLOGE("dlopen %s", path);
                continue;
            }

            auto *module = new struct module(strdup(entry->d_name));
            module->handle = handle;
            module->onModuleLoaded = dlsym(handle, "onModuleLoaded");
            module->forkAndSpecializePre = dlsym(handle, "nativeForkAndSpecializePre");
            module->forkAndSpecializePost = dlsym(handle, "nativeForkAndSpecializePost");
            module->forkSystemServerPre = dlsym(handle, "nativeForkSystemServerPre");
            module->forkSystemServerPost = dlsym(handle, "nativeForkSystemServerPost");
            module->shouldSkipUid = dlsym(handle, "shouldSkipUid");
            get_modules()->push_back(module);

            if (moduleApiVersion == -1) {
                // only for api v2
                module->getApiVersion = dlsym(handle, "getApiVersion");

                if (module->getApiVersion) {
                    module->apiVersion = ((getApiVersion_t) module->getApiVersion)();
                }
            } else {
                module->apiVersion = moduleApiVersion;
            }

            void *sym = dlsym(handle, "riru_set_module_name");
            if (sym)
                ((void (*)(const char *)) sym)(module->name);

#ifdef __LP64__
            LOGI("module loaded: %s (api %d)", module->name, module->apiVersion);
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

static JNINativeMethod *onRegisterZygote(JNIEnv *env, const char *className,
                                         const JNINativeMethod *methods, int numMethods) {
    int replaced = 0;

    auto *newMethods = new JNINativeMethod[numMethods];
    memcpy(newMethods, methods, sizeof(JNINativeMethod) * numMethods);

    JNINativeMethod method;
    for (int i = 0; i < numMethods; ++i) {
        method = methods[i];

        if (strcmp(method.name, "nativeForkAndSpecialize") == 0) {
            set_nativeForkAndSpecialize(method.fnPtr);

            if (strcmp(nativeForkAndSpecialize_marshmallow_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkAndSpecialize_marshmallow;
            else if (strcmp(nativeForkAndSpecialize_oreo_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkAndSpecialize_oreo;
            else if (strcmp(nativeForkAndSpecialize_p_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkAndSpecialize_p;
            else if (strcmp(nativeForkAndSpecialize_q_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkAndSpecialize_q;
            else if (strcmp(nativeForkAndSpecialize_samsung_p_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkAndSpecialize_samsung_p;
            else if (strcmp(nativeForkAndSpecialize_samsung_o_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkAndSpecialize_samsung_o;
            else if (strcmp(nativeForkAndSpecialize_samsung_n_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkAndSpecialize_samsung_n;
            else if (strcmp(nativeForkAndSpecialize_samsung_m_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkAndSpecialize_samsung_m;
            else
                LOGW("found nativeForkAndSpecialize but signature %s mismatch", method.signature);

            if (newMethods[i].fnPtr != methods[i].fnPtr) {
                LOGI("replaced com.android.internal.os.Zygote#nativeForkAndSpecialize");
                riru_set_native_method_func(MODULE_NAME_CORE, className, newMethods[i].name,
                                            newMethods[i].signature, newMethods[i].fnPtr);

                replaced += 1;
            }
        } else if (strcmp(method.name, "nativeForkSystemServer") == 0) {
            set_nativeForkSystemServer(method.fnPtr);

            if (strcmp(nativeForkSystemServer_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkSystemServer;
            else
                LOGW("found nativeForkSystemServer but signature %s mismatch", method.signature);

            if (newMethods[i].fnPtr != methods[i].fnPtr) {
                LOGI("replaced com.android.internal.os.Zygote#nativeForkSystemServer");
                riru_set_native_method_func(MODULE_NAME_CORE, className, newMethods[i].name,
                                            newMethods[i].signature, newMethods[i].fnPtr);

                replaced += 1;
            }
        }
    }

    methods_replaced = replaced == 2;

    return newMethods;
}

static JNINativeMethod *onRegisterSystemProperties(JNIEnv *env, const char *className,
                                                   const JNINativeMethod *methods, int numMethods) {
    auto *newMethods = new JNINativeMethod[numMethods];
    memcpy(newMethods, methods, sizeof(JNINativeMethod) * numMethods);

    JNINativeMethod method;
    for (int i = 0; i < numMethods; ++i) {
        method = methods[i];

        if (strcmp(method.name, "native_set") == 0) {
            set_SystemProperties_set(method.fnPtr);

            if (strcmp("(Ljava/lang/String;Ljava/lang/String;)V", method.signature) == 0)
                newMethods[i].fnPtr = (void *) SystemProperties_set;
            else
                LOGW("found native_set but signature %s mismatch", method.signature);

            if (newMethods[i].fnPtr != methods[i].fnPtr) {
                LOGI("replaced android.os.SystemProperties#native_set");

                riru_set_native_method_func(MODULE_NAME_CORE, className, newMethods[i].name,
                                            newMethods[i].signature, newMethods[i].fnPtr);
            }
        }
    }
    return newMethods;
}

#define XHOOK_REGISTER(PATH_REGEX, NAME) \
    if (xhook_register(PATH_REGEX, #NAME, (void*) new_##NAME, (void **) &old_##NAME) != 0) \
        LOGE("failed to register hook " #NAME "."); \

#define NEW_FUNC_DEF(ret, func, ...) \
    static ret (*old_##func)(__VA_ARGS__); \
    static ret new_##func(__VA_ARGS__)

NEW_FUNC_DEF(int, jniRegisterNativeMethods, JNIEnv *env, const char *className,
             const JNINativeMethod *methods, int numMethods) {
    put_native_method(className, methods, numMethods);

    LOGV("jniRegisterNativeMethods %s", className);

    JNINativeMethod *newMethods = nullptr;
    if (strcmp("com/android/internal/os/Zygote", className) == 0) {
        newMethods = onRegisterZygote(env, className, methods, numMethods);
    } else if (strcmp("android/os/SystemProperties", className) == 0) {
        // hook android.os.SystemProperties#native_set to prevent a critical problem on Android 9+
        // see comment of SystemProperties_set in jni_native_method.cpp for detail
        newMethods = onRegisterSystemProperties(env, className, methods, numMethods);
    }

    int res = old_jniRegisterNativeMethods(env, className, newMethods ? newMethods : methods,
                                           numMethods);
    delete newMethods;
    return res;
}

void unhook_jniRegisterNativeMethods() {
    xhook_register(".*\\libandroid_runtime.so$", "jniRegisterNativeMethods",
                   (void *) old_jniRegisterNativeMethods,
                   nullptr);
    if (xhook_refresh(0) == 0) {
        xhook_clear();
        LOGI("hook removed");
    }
}

static void read_prop() {
    char sdk[PROP_VALUE_MAX + 1];
    if (__system_property_get("ro.build.version.sdk", sdk) > 0)
        sdkLevel = atoi(sdk);

    __system_property_get("ro.build.version.release", androidVersionName);

    LOGI("system version %s (api %d)", androidVersionName, sdkLevel);
}

extern "C" void constructor() __attribute__((constructor));

void constructor() {
    static int loaded = 0;
    if (loaded)
        return;

    loaded = 1;

    if (getuid() != 0)
        return;

    char cmdline[ARG_MAX + 1];
    get_self_cmdline(cmdline);

    if (!strstr(cmdline, "--zygote"))
        return;

#ifdef __LP64__
    LOGI("riru in zygote64");
#else
    LOGI("riru in zygote");
#endif

    if (access(CONFIG_DIR "/.disable", F_OK) == 0) {
        LOGI(CONFIG_DIR "/.disable exists, do nothing.");
        return;
    }

    read_prop();

    XHOOK_REGISTER(".*\\libandroid_runtime.so$", jniRegisterNativeMethods);

    if (xhook_refresh(0) == 0) {
        xhook_clear();
        LOGI("hook installed");
    } else {
        LOGE("failed to refresh hook");
    }

    load_modules();
}