#ifndef RIRU_H
#define RIRU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <jni.h>
#include <sys/types.h>

// ---------------------------------------------------------

typedef void(onModuleLoaded_v9)();

typedef int(shouldSkipUid_v9)(int uid);

typedef void(nativeForkAndSpecializePre_v9)(
        JNIEnv *env, jclass cls, jint *uid, jint *gid, jintArray *gids, jint *runtimeFlags,
        jobjectArray *rlimits, jint *mountExternal, jstring *seInfo, jstring *niceName,
        jintArray *fdsToClose, jintArray *fdsToIgnore, jboolean *is_child_zygote,
        jstring *instructionSet, jstring *appDataDir, jboolean *isTopApp, jobjectArray *pkgDataInfoList,
        jobjectArray *whitelistedDataInfoList, jboolean *bindMountAppDataDirs, jboolean *bindMountAppStorageDirs);

typedef void(nativeForkAndSpecializePost_v9)(JNIEnv *env, jclass cls, jint res);

typedef void(nativeForkSystemServerPre_v9)(
        JNIEnv *env, jclass cls, uid_t *uid, gid_t *gid, jintArray *gids, jint *runtimeFlags,
        jobjectArray *rlimits, jlong *permittedCapabilities, jlong *effectiveCapabilities);

typedef void(nativeForkSystemServerPost_v9)(JNIEnv *env, jclass cls, jint res);

typedef void(nativeSpecializeAppProcessPre_v9)(
        JNIEnv *env, jclass cls, jint *uid, jint *gid, jintArray *gids, jint *runtimeFlags,
        jobjectArray *rlimits, jint *mountExternal, jstring *seInfo, jstring *niceName,
        jboolean *startChildZygote, jstring *instructionSet, jstring *appDataDir,
        jboolean *isTopApp, jobjectArray *pkgDataInfoList, jobjectArray *whitelistedDataInfoList,
        jboolean *bindMountAppDataDirs, jboolean *bindMountAppStorageDirs);

typedef void(nativeSpecializeAppProcessPost_v9)(JNIEnv *env, jclass cls);

typedef struct {
    int supportHide;
    int version;
    const char *versionName;
    onModuleLoaded_v9 *onModuleLoaded;
    shouldSkipUid_v9 *shouldSkipUid;
    nativeForkAndSpecializePre_v9 *forkAndSpecializePre;
    nativeForkAndSpecializePost_v9 *forkAndSpecializePost;
    nativeForkSystemServerPre_v9 *forkSystemServerPre;
    nativeForkSystemServerPost_v9 *forkSystemServerPost;
    nativeSpecializeAppProcessPre_v9 *specializeAppProcessPre;
    nativeSpecializeAppProcessPost_v9 *specializeAppProcessPost;
} RiruModuleInfo;

typedef struct {
    int moduleApiVersion;
    RiruModuleInfo moduleInfo;
} RiruVersionedModuleInfo;

// ---------------------------------------------------------

typedef void *(RiruGetFunc_v9)(uint32_t token, const char *name);

typedef void (RiruSetFunc_v9)(uint32_t token, const char *name, void *func);

typedef void *(RiruGetJNINativeMethodFunc_v9)(uint32_t token, const char *className, const char *name, const char *signature);

typedef void (RiruSetJNINativeMethodFunc_v9)(uint32_t token, const char *className, const char *name, const char *signature,
                                             void *func);

typedef const JNINativeMethod *(RiruGetOriginalJNINativeMethodFunc_v9)(const char *className, const char *name,
                                                                       const char *signature);

typedef void *(RiruGetGlobalValue_v9)(const char *key);

typedef void(RiruPutGlobalValue_v9)(const char *key, void *value);

typedef struct {
    uint32_t token;
    RiruGetFunc_v9 *getFunc;
    RiruGetJNINativeMethodFunc_v9 *getJNINativeMethodFunc;
    RiruSetFunc_v9 *setFunc;
    RiruSetJNINativeMethodFunc_v9 *setJNINativeMethodFunc;
    RiruGetOriginalJNINativeMethodFunc_v9 *getOriginalJNINativeMethodFunc;
    RiruGetGlobalValue_v9 *getGlobalValue;
    RiruPutGlobalValue_v9 *putGlobalValue;
} RiruApi;

typedef struct {
    int riruApiVersion;
    RiruApi *riruApi;
    const char *magiskModulePath;
} Riru;

typedef RiruVersionedModuleInfo *(RiruInit_t)(Riru *);

#ifdef RIRU_MODULE
#define RIRUD_ADDRESS "rirud"

#define RIRU_EXPORT __attribute__((visibility("default"))) __attribute__((used))

RiruVersionedModuleInfo *init(Riru *riru) RIRU_EXPORT;

extern int riru_api_version;
extern RiruApi *riru_api;
extern const char *riru_magisk_module_path;

#if !defined(__cplusplus)
#define inline attribute(inline)
#endif

inline const char *riru_get_magisk_module_path() {
    if (riru_api_version >= 24) {
        return riru_magisk_module_path;
    }
    return NULL;
}

inline void *riru_get_func(const char *name) {
    return riru_api->getFunc(riru_api->token, name);
}

inline void *riru_get_native_method_func(const char *className, const char *name, const char *signature) {
    return riru_api->getJNINativeMethodFunc(riru_api->token, className, name, signature);
}

inline const JNINativeMethod *riru_get_original_native_methods(const char *className, const char *name, const char *signature) {
    return riru_api->getOriginalJNINativeMethodFunc(className, name, signature);
}

inline void riru_set_func(const char *name, void *func) {
    riru_api->setFunc(riru_api->token, name, func);
}

inline void riru_set_native_method_func(const char *className, const char *name, const char *signature, void *func) {
    riru_api->setJNINativeMethodFunc(riru_api->token, className, name, signature, func);
}

inline void *riru_get_global_value(const char *key) {
    return riru_api->getGlobalValue(key);
}

inline void riru_put_global_value(const char *key, void *value) {
    riru_api->putGlobalValue(key, value);
}

#endif

#ifdef __cplusplus
}
#endif

#endif //RIRU_H