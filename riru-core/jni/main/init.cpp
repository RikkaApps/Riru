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

std::vector<module *> *get_modules() {
    static auto *modules = new std::vector<module *>();
    return modules;
}

unsigned long get_module_index(const char *name) {
    if (!name)
        return 0;

    for (unsigned long i = 0; i < get_modules()->size(); ++i) {
        if (strcmp(get_modules()->at(i)->name, name) == 0)
            return i + 1;
    }
    return 0;
}

static auto *native_methods = new std::map<std::string, std::pair<const JNINativeMethod *, int>>();

extern "C" {
__attribute__((visibility("default")))
const JNINativeMethod *get_native_method(const char *className, const char *name,
                                         const char *signature) {
    auto it = native_methods->find(className);
    if (it != native_methods->end()) {
        auto pair = it->second;
        for (int i = 0; i < pair.second; ++i) {
            auto method = &pair.first[i];
            if (strcmp(method->name, name) == 0 && strcmp(method->signature, signature) == 0)
                return method;
        }
    }
    return nullptr;
}

const JNINativeMethod *get_native_methods(const char *className) {
    auto it = native_methods->find(className);
    if (it != native_methods->end()) {
        return it->second.first;
    }
    return nullptr;
}

void *riru_get_func(const char *module_name, const char *name);
void *riru_get_native_method_func(const char *module_name, const char *className, const char *name,
                             const char *signature);
void riru_set_func(const char *module_name, const char *name, void* func);
void riru_set_native_method_func(const char *module_name, const char *className, const char *name,
                            const char *signature, void* func);
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

JNINativeMethod gZygoteMethods[] = {{nullptr, nullptr, nullptr},
                                    {nullptr, nullptr, nullptr}};

void *_nativeForkAndSpecialize = nullptr;
void *_nativeForkSystemServer = nullptr;

JNINativeMethod gSystemPropertiesMethods[] = {{nullptr, nullptr, nullptr}};
void *_SystemProperties_set = nullptr;

void onRegisterZygote(JNIEnv *env, const char *className, const JNINativeMethod *methods,
                      int numMethods) {
    JNINativeMethod method;
    for (int i = 0; i < numMethods; ++i) {
        method = methods[i];

        if (strcmp(method.name, "nativeForkAndSpecialize") == 0) {
            _nativeForkAndSpecialize = method.fnPtr;

            gZygoteMethods[0].name = method.name;
            gZygoteMethods[0].signature = method.signature;

            if (strncmp(nativeForkAndSpecialize_marshmallow_sig, method.signature,
                        strlen(nativeForkAndSpecialize_marshmallow_sig)) == 0)
                gZygoteMethods[0].fnPtr = (void *) nativeForkAndSpecialize_marshmallow;
            else if (strncmp(nativeForkAndSpecialize_oreo_sig, method.signature,
                             strlen(nativeForkAndSpecialize_oreo_sig)) == 0)
                gZygoteMethods[0].fnPtr = (void *) nativeForkAndSpecialize_oreo;
            else if (strncmp(nativeForkAndSpecialize_p_sig, method.signature,
                             strlen(nativeForkAndSpecialize_p_sig)) == 0)
                gZygoteMethods[0].fnPtr = (void *) nativeForkAndSpecialize_p;
        } else if (strcmp(method.name, "nativeForkSystemServer") == 0) {
            _nativeForkSystemServer = method.fnPtr;

            gZygoteMethods[1].name = method.name;
            gZygoteMethods[1].signature = method.signature;

            if (strncmp(nativeForkSystemServer_sig, method.signature,
                        strlen(nativeForkSystemServer_sig)) == 0)
                gZygoteMethods[1].fnPtr = (void *) nativeForkSystemServer;
        }
    }

    LOGI("{\"%s\", \"%s\", %p}", gZygoteMethods[0].name, gZygoteMethods[0].signature,
         gZygoteMethods[0].fnPtr);
    LOGI("{\"%s\", \"%s\", %p}", gZygoteMethods[1].name, gZygoteMethods[1].signature,
         gZygoteMethods[1].fnPtr);

    if (gZygoteMethods[0].fnPtr && gZygoteMethods[1].fnPtr) {
        jclass clazz = JNI_FindClass(env, className);
        if (clazz) {
            jint r = JNI_RegisterNatives(env, clazz, gZygoteMethods, 2);
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

void onRegisterSystemProperties(JNIEnv *env, const char *className, const JNINativeMethod *methods,
                                int numMethods) {
    JNINativeMethod method;
    for (int i = 0; i < numMethods; ++i) {
        method = methods[i];

        if (strcmp(method.name, "native_set") == 0) {
            _SystemProperties_set = method.fnPtr;

            gSystemPropertiesMethods[0].name = method.name;
            gSystemPropertiesMethods[0].signature = method.signature;

            if (strcmp("(Ljava/lang/String;Ljava/lang/String;)V", method.signature) == 0)
                gSystemPropertiesMethods[0].fnPtr = (void *) SystemProperties_set;
        }
    }

    LOGI("{\"%s\", \"%s\", %p}", gSystemPropertiesMethods[0].name,
         gSystemPropertiesMethods[0].signature,
         gSystemPropertiesMethods[0].fnPtr);

    if (gSystemPropertiesMethods[0].fnPtr) {
        jclass clazz = JNI_FindClass(env, className);
        if (clazz) {
            jint r = JNI_RegisterNatives(env, clazz, gSystemPropertiesMethods, 1);
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
    (*native_methods)[className] = std::pair<const JNINativeMethod *, int>(methods, numMethods);

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

extern "C" void con() __attribute__((constructor));

void con() {
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

void *riru_get_func(const char *module_name, const char *name) {
    unsigned long index = get_module_index(module_name);
    if (index == 0)
        return nullptr;

    index -= 1;

    LOGI("get_func %s %s", module_name, name);

    // find if it is set by previous modules
    if (index != 0) {
        for (unsigned long i = index - 1; i >= 0; --i) {
            auto module = get_modules()->at(i);
            auto it = module->funcs->find(name);
            if (module->funcs->end() != it)
                return it->second;

            if (i == 0) break;
        }
    }

    return nullptr;
}

void *riru_get_native_method_func(const char *module_name, const char *className, const char *name,
                                  const char *signature) {
    unsigned long index = get_module_index(module_name);
    if (index == 0)
        return nullptr;

    index -= 1;

    LOGI("get_func %s %s %s %s", module_name, className, name, signature);

    // find if it is set by previous modules
    if (index != 0) {
        for (unsigned long i = index - 1; i >= 0; --i) {
            auto module = get_modules()->at(i);
            auto it = module->funcs->find(std::string(className) + name + signature);
            if (module->funcs->end() != it)
                return it->second;

            if (i == 0) break;
        }
    }

    const JNINativeMethod *jniNativeMethod = get_native_method(className, name, signature);
    return jniNativeMethod ? jniNativeMethod->fnPtr : nullptr;
}

void riru_set_func(const char *module_name, const char *name, void* func) {
    unsigned long index = get_module_index(module_name);
    if (index == 0)
        return;

    LOGI("set_func %s %s %p", module_name, name, func);

    auto module = get_modules()->at(index - 1);
    (*module->funcs)[name] = func;
}

void riru_set_native_method_func(const char *module_name, const char *className, const char *name,
                                 const char *signature, void* func) {
    riru_set_func(module_name, (std::string(className) + name + signature).c_str(), func);
}