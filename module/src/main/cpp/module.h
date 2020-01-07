#ifndef MODULE_H
#define MODULE_H

#include <jni.h>
#include <map>
#include <vector>

#define MODULE_NAME_CORE "core"

using loaded_t=void();

// ---------------------------------------------------------

using nativeForkAndSpecialize_pre_t = void(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jintArray, jintArray, jboolean, jstring, jstring);

using nativeForkAndSpecialize_pre_v2_t = void(
        JNIEnv *, jclass, jint *, jint *, jintArray *, jint *, jobjectArray *, jint *, jstring *,
        jstring *, jintArray *, jintArray *, jboolean *, jstring *, jstring *);

using nativeForkAndSpecialize_pre_v3_t = void(
        JNIEnv *, jclass, jint *, jint *, jintArray *, jint *, jobjectArray *, jint *, jstring *,
        jstring *, jintArray *, jintArray *, jboolean *, jstring *, jstring *, jstring *,
        jobjectArray *, jstring *);

using nativeForkAndSpecialize_post_t = int(
        JNIEnv *, jclass, jint);

// ---------------------------------------------------------

using nativeForkSystemServer_pre_t = void(
        JNIEnv *, jclass, uid_t, gid_t, jintArray,
        jint, jobjectArray, jlong, jlong);

using nativeForkSystemServer_pre_v2_t = void(
        JNIEnv *, jclass, uid_t *, gid_t *, jintArray *, jint *, jobjectArray *, jlong *, jlong *);

using nativeForkSystemServer_post_t = int(JNIEnv *, jclass, jint);

// ---------------------------------------------------------

using nativeSpecializeAppProcess_pre_t = void(
        JNIEnv *, jclass, jint *, jint *, jintArray *, jint *, jobjectArray *, jint *, jstring *,
        jstring *, jboolean *, jstring *, jstring *, jstring *, jobjectArray *, jstring *);

using nativeSpecializeAppProcess_post_t = int(JNIEnv *, jclass);

using shouldSkipUid_t = int(int);

using getApiVersion_t = int();

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
