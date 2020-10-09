#pragma once

#include <jni.h>
#include <string>
#include <map>
#include <vector>
#include "api.h"

#define MODULE_NAME_CORE "core"

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

using nativeForkAndSpecialize_pre_v5_t = void(
        JNIEnv *, jclass, jint *, jint *, jintArray *, jint *, jobjectArray *, jint *, jstring *,
        jstring *, jintArray *, jintArray *, jboolean *, jstring *, jstring *, jboolean *,
        jobjectArray *);

using nativeForkAndSpecialize_pre_v6_t = void(
        JNIEnv *, jclass, jint *, jint *, jintArray *, jint *, jobjectArray *, jint *, jstring *,
        jstring *, jintArray *, jintArray *, jboolean *, jstring *, jstring *, jboolean *,
        jobjectArray *, jboolean *);

using nativeForkAndSpecialize_pre_v7_t = void(
        JNIEnv *, jclass, jint *, jint *, jintArray *, jint *, jobjectArray *, jint *, jstring *,
        jstring *, jintArray *, jintArray *, jboolean *, jstring *, jstring *, jboolean *,
        jobjectArray *, jobjectArray *, jboolean *, jboolean *);

using nativeForkAndSpecialize_post_t = int(
        JNIEnv *, jclass, jint);

// ---------------------------------------------------------

using nativeForkSystemServer_pre_t = void(
        JNIEnv *, jclass, uid_t, gid_t, jintArray, jint, jobjectArray, jlong, jlong);

using nativeForkSystemServer_pre_v2_t = void(
        JNIEnv *, jclass, uid_t *, gid_t *, jintArray *, jint *, jobjectArray *, jlong *, jlong *);

using nativeForkSystemServer_post_t = int(JNIEnv *, jclass, jint);

// ---------------------------------------------------------

using nativeSpecializeAppProcess_pre_v4_t = void(
        JNIEnv *, jclass, jint *, jint *, jintArray *, jint *, jobjectArray *, jint *, jstring *,
        jstring *, jboolean *, jstring *, jstring *, jstring *, jobjectArray *, jstring *);

using nativeSpecializeAppProcess_pre_v5_t = void(
        JNIEnv *, jclass, jint *, jint *, jintArray *, jint *, jobjectArray *, jint *, jstring *,
        jstring *, jboolean *, jstring *, jstring *, jboolean *, jobjectArray *);

using nativeSpecializeAppProcess_pre_v6_t = void(
        JNIEnv *, jclass, jint *, jint *, jintArray *, jint *, jobjectArray *, jint *, jstring *,
        jstring *, jboolean *, jstring *, jstring *, jboolean *, jobjectArray *, jboolean *);

using nativeSpecializeAppProcess_pre_v7_t = void(
        JNIEnv *, jclass, jint *, jint *, jintArray *, jint *, jobjectArray *, jint *, jstring *,
        jstring *, jboolean *, jstring *, jstring *, jboolean *, jobjectArray *, jobjectArray *,
        jboolean *, jboolean *);

using nativeSpecializeAppProcess_post_t = int(JNIEnv *, jclass);

// ---------------------------------------------------------

struct RiruModuleExt : RiruModule {

    void *handle{};
    const char *name;
    std::map<std::string, void *> *funcs;
    uint32_t token;

    explicit RiruModuleExt(const char *name) : name(name) {
        funcs = new std::map<std::string, void *>();
        token = (uintptr_t) name + (uintptr_t) funcs;
    }
};


std::vector<RiruModuleExt *> *get_modules();

void load_modules();