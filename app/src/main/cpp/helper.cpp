#include <jni.h>
#include <elf.h>
#include <link.h>
#include <cassert>
#include <sys/mman.h>
#include <dlfcn.h>
#include "logging.h"

extern "C" {
#include "pmparser.h"
}

static jboolean is_path_in_maps(const char *path) {
    procmaps_iterator *maps = pmparser_parse(-1);
    if (maps == nullptr) {
        LOGE("[map]: cannot parse the memory map");
        return JNI_FALSE;
    }

    procmaps_struct *maps_tmp = nullptr;
    while ((maps_tmp = pmparser_next(maps)) != nullptr) {
        if (strstr(maps_tmp->pathname, path))
            return JNI_TRUE;
    }
    pmparser_free(maps);
    return JNI_FALSE;
}

static jboolean init(JNIEnv *env, jobject thiz) {
    procmaps_iterator *maps = pmparser_parse(-1);
    if (maps == nullptr) {
        LOGE("[map]: cannot parse the memory map");
        return JNI_FALSE;
    }

    jboolean res = JNI_FALSE;
    procmaps_struct *maps_tmp = nullptr;
    while ((maps_tmp = pmparser_next(maps)) != nullptr) {
        //LOGI("%s", maps_tmp->pathname);
        if (strstr(maps_tmp->pathname, "libmemtrack_real.so")) {
            res = JNI_TRUE;
        }
    }
    pmparser_free(maps);
    return res;
}

static jboolean is_riru_module_exists(JNIEnv *env, jobject thiz, jstring name) {
    // TODO
    return JNI_FALSE;
}

static void *handle;

static void *get_handle() {
    if (handle == nullptr)
        handle = dlopen(nullptr, 0);

    return handle;
}

static jint get_riru_rersion(JNIEnv *env, jobject thiz) {
    static void *sym;
    void *handle;
    if ((handle = get_handle()) == nullptr) return -1;
    if (sym == nullptr) sym = dlsym(handle, "riru_get_version");
    if (sym) return ((int (*)()) sym)();
    return -1;
}

static jboolean is_zygote_methods_replaced(JNIEnv *env, jobject thiz) {
    static void *sym;
    void *handle;
    if ((handle = get_handle()) == nullptr) return JNI_FALSE;
    if (sym == nullptr) sym = dlsym(handle, "riru_is_zygote_methods_replaced");
    if (sym) return static_cast<jboolean>(((int (*)()) sym)());
    return JNI_FALSE;
}

static jint get_nativeForkAndSpecialize_calls_count(JNIEnv *env, jobject thiz) {
    static void *sym;
    void *handle;
    if ((handle = get_handle()) == nullptr) return -1;
    if (sym == nullptr) sym = dlsym(handle, "riru_get_nativeForkAndSpecialize_calls_count");
    if (sym) return static_cast<jboolean>(((int (*)()) sym)());
    return -1;
}

static jint get_nativeForkSystemServer_calls_count(JNIEnv *env, jobject thiz) {
    static void *sym;
    void *handle;
    if ((handle = get_handle()) == nullptr) return -1;
    if (sym == nullptr) sym = dlsym(handle, "riru_get_nativeForkSystemServer_calls_count");
    if (sym) return static_cast<jboolean>(((int (*)()) sym)());
    return -1;
}

static jint get_nativeSpecializeAppProcess_calls_count(JNIEnv *env, jobject thiz) {
    static void *sym;
    void *handle;
    if ((handle = get_handle()) == nullptr) return -1;
    if (sym == nullptr) sym = dlsym(handle, "riru_get_nativeSpecializeAppProcess_calls_count");
    if (sym) return static_cast<jboolean>(((int (*)()) sym)());
    return -1;
}

static jstring get_nativeForkAndSpecialize_signature(JNIEnv *env, jobject thiz) {
    static void *sym;
    void *handle;
    if ((handle = get_handle()) == nullptr) return nullptr;
    if (sym == nullptr) sym = dlsym(handle, "riru_get_original_native_methods");
    if (sym) {
        auto method = ((const JNINativeMethod *(*)(const char *, const char *, const char *)) sym)(
                "com/android/internal/os/Zygote", "nativeForkAndSpecialize", nullptr);
        if (method != nullptr)
            return env->NewStringUTF(method->signature);
    }
    return nullptr;
}

static jstring get_nativeSpecializeAppProcess_signature(JNIEnv *env, jobject thiz) {
    static void *sym;
    void *handle;
    if ((handle = get_handle()) == nullptr) return nullptr;
    if (sym == nullptr) sym = dlsym(handle, "riru_get_original_native_methods");
    if (sym) {
        auto method = ((const JNINativeMethod *(*)(const char *, const char *, const char *)) sym)(
                "com/android/internal/os/Zygote", "nativeSpecializeAppProcess", nullptr);
        if (method != nullptr)
            return env->NewStringUTF(method->signature);
    }
    return nullptr;
}

static jstring get_nativeForkSystemServer_signature(JNIEnv *env, jobject thiz) {
    static void *sym;
    void *handle;
    if ((handle = get_handle()) == nullptr) return nullptr;
    if (sym == nullptr) sym = dlsym(handle, "riru_get_original_native_methods");
    if (sym) {
        auto method = ((const JNINativeMethod *(*)(const char *, const char *, const char *)) sym)(
                "com/android/internal/os/Zygote", "nativeForkSystemServer", nullptr);
        if (method != nullptr)
            return env->NewStringUTF(method->signature);
    }
    return nullptr;
}

static JNINativeMethod gMethods[] = {
        {"init",                                    "()Z",                   (void *) init},
        {"isRiruModuleExists",                      "(Ljava/lang/String;)Z", (void *) is_riru_module_exists},
        {"getRiruVersion",                          "()I",                   (void *) get_riru_rersion},
        {"isZygoteMethodsReplaced",                 "()Z",                   (void *) is_zygote_methods_replaced},
        {"getNativeForkAndSpecializeCallsCount",    "()I",                   (void *) get_nativeForkAndSpecialize_calls_count},
        {"getNativeForkSystemServerCallsCount",     "()I",                   (void *) get_nativeForkSystemServer_calls_count},
        {"getNativeSpecializeAppProcessCallsCount", "()I",                   (void *) get_nativeSpecializeAppProcess_calls_count},
        {"getNativeForkAndSpecializeSignature",     "()Ljava/lang/String;",  (void *) get_nativeForkAndSpecialize_signature},
        {"getNativeSpecializeAppProcessSignature",  "()Ljava/lang/String;",  (void *) get_nativeSpecializeAppProcess_signature},
        {"getNativeForkSystemServerSignature",      "()Ljava/lang/String;",  (void *) get_nativeForkSystemServer_signature},
};

static int registerNativeMethods(JNIEnv *env, const char *className,
                                 JNINativeMethod *gMethods, int numMethods) {
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == nullptr)
        return JNI_FALSE;

    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0)
        return JNI_FALSE;

    return JNI_TRUE;
}

static int registerNatives(JNIEnv *env) {
    if (!registerNativeMethods(env, "moe/riru/manager/utils/NativeHelper", gMethods,
                               sizeof(gMethods) / sizeof(gMethods[0])))
        return JNI_FALSE;

    return JNI_TRUE;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = nullptr;
    jint result;

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK)
        return -1;

    assert(env != nullptr);

    if (!registerNatives(env)) {
        LOGE("registerNatives NativeHelper");
        return -1;
    }

    result = JNI_VERSION_1_6;

    return result;
}
