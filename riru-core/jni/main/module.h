#ifndef MODULE_H
#define MODULE_H

#include <jni.h>
#include <map>
#include <vector>

#define MODULE_NAME_CORE "core"

typedef void (*loaded_t)();

// ---------------------------------------------------------

typedef void (*nativeForkAndSpecialize_pre_t)(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jintArray, jintArray, jboolean, jstring, jstring);

typedef void (*nativeForkAndSpecialize_pre_v2_t)(
        JNIEnv *, jclass, jint *, jint *, jintArray *, jint *, jobjectArray *, jint *, jstring *,
        jstring *, jintArray *, jintArray *, jboolean *, jstring *, jstring *);

typedef void (*nativeForkAndSpecialize_pre_v3_t)(
        JNIEnv *, jclass, jint *, jint *, jintArray *, jint *, jobjectArray *, jint *, jstring *,
        jstring *, jintArray *, jintArray *, jboolean *, jstring *, jstring *, jstring *,
        jobjectArray *, jstring *);

typedef int (*nativeForkAndSpecialize_post_t)(
        JNIEnv *, jclass, jint);

// ---------------------------------------------------------

typedef void (*nativeForkSystemServer_pre_t)(
        JNIEnv *, jclass, uid_t, gid_t, jintArray,
        jint, jobjectArray, jlong, jlong);

typedef void (*nativeForkSystemServer_pre_v2_t)(
        JNIEnv *, jclass, uid_t *, gid_t *, jintArray *, jint *, jobjectArray *, jlong *, jlong *);

typedef int (*nativeForkSystemServer_post_t)(
        JNIEnv *, jclass, jint);

// ---------------------------------------------------------

typedef void (*nativeSpecializeAppProcess_pre_t)(
        JNIEnv *, jclass, jint *, jint *, jintArray *, jint *, jobjectArray *, jint *, jstring *,
        jstring *, jboolean *, jstring *, jstring *, jstring *, jobjectArray *, jstring *);

typedef int (*nativeSpecializeAppProcess_post_t)(
        JNIEnv *, jclass);

typedef int (*shouldSkipUid_t)(int);

typedef int (*getApiVersion_t)();

struct module {
    void *handle{};
    char *name;
    void *onModuleLoaded{};
    void *forkAndSpecializePre{};
    void *forkAndSpecializePost{};
    void *forkSystemServerPre{};
    void *forkSystemServerPost{};
    void *specializeAppProcessPre{};
    void *specializeAppProcessPost{};
    void *shouldSkipUid{};
    void *getApiVersion{};
    int apiVersion = 0;
    std::map<std::string, void *> *funcs;

    explicit module(char *name) : name(name) {
        funcs = new std::map<std::string, void *>();
    }
};

std::vector<module *> *get_modules();

void put_native_method(const char *className, const JNINativeMethod *methods, int numMethods);

#endif // MODULE_H
