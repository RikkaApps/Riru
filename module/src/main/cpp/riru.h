#ifndef RIRU_H
#define RIRU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// ---------------------------------------------------------

typedef void(onModuleLoaded_t)();

typedef int(shouldSkipUid_t)(int uid);

typedef void(nativeForkAndSpecializePre_t)(
        JNIEnv *env, jclass clazz, jint *uid, jint *gid, jintArray *gids, jint *runtimeFlags,
        jobjectArray *rlimits, jint *mountExternal, jstring *seInfo, jstring *niceName,
        jintArray *fdsToClose, jintArray *fdsToIgnore, jboolean *is_child_zygote,
        jstring *instructionSet, jstring *appDataDir, jboolean *isTopApp, jobjectArray *pkgDataInfoList,
        jobjectArray *whitelistedDataInfoList, jboolean *bindMountAppDataDirs, jboolean *bindMountAppStorageDirs);

typedef void(nativeForkAndSpecializePost_t)(JNIEnv *env, jclass cls, jint res);

typedef void(nativeForkSystemServerPre_t)(
        JNIEnv *env, jclass cls, uid_t *uid, gid_t *gid, jintArray *gids, jint *runtimeFlags,
        jobjectArray *rlimits, jlong *permittedCapabilities, jlong *effectiveCapabilities);

typedef int(nativeForkSystemServerPost_t)(JNIEnv *env, jclass cls, jint res);

typedef void(nativeSpecializeAppProcessPre_t)(
        JNIEnv *env, jclass clazz, jint *uid, jint *gid, jintArray *gids, jint *runtimeFlags,
        jobjectArray *rlimits, jint *mountExternal, jstring *seInfo, jstring *niceName,
        jboolean *startChildZygote, jstring *instructionSet, jstring *appDataDir,
        jboolean *isTopApp, jobjectArray *pkgDataInfoList, jobjectArray *whitelistedDataInfoList,
        jboolean *bindMountAppDataDirs, jboolean *bindMountAppStorageDirs);

typedef int(nativeSpecializeAppProcessPost_t)(JNIEnv *, jclass);

struct RiruModule {

    int apiVersion = 0;
    int supportHide = 0;
    onModuleLoaded_t *onModuleLoaded = nullptr;
    shouldSkipUid_t *shouldSkipUid = nullptr;
    nativeForkAndSpecializePre_t *forkAndSpecializePre = nullptr;
    nativeForkAndSpecializePost_t *forkAndSpecializePost = nullptr;
    nativeForkSystemServerPre_t *forkSystemServerPre = nullptr;
    nativeForkSystemServerPost_t *forkSystemServerPost = nullptr;
    nativeSpecializeAppProcessPre_t *specializeAppProcessPre = nullptr;
    nativeSpecializeAppProcessPost_t *specializeAppProcessPost = nullptr;
};

// ---------------------------------------------------------

typedef void *(RiruGetFunc_t)(uint32_t token, const char *name);

typedef void (RiruSetFunc_t)(uint32_t token, const char *name, void *func);
typedef void *(RiruGetJNINativeMethodFunc_t)(uint32_t token, const char *className, const char *name, const char *signature);
typedef void (RiruSetJNINativeMethodFunc_t)(uint32_t token, const char *className, const char *name, const char *signature, void *func);
typedef const JNINativeMethod *(RiruGetOriginalJNINativeMethodFunc_t)(const char *className, const char *name, const char *signature);

struct RiruFuncs {
    RiruGetFunc_t *getFunc;
    RiruGetJNINativeMethodFunc_t *getJNINativeMethodFunc;
    RiruSetFunc_t *setFunc;
    RiruSetJNINativeMethodFunc_t *setJNINativeMethodFunc;
    RiruGetOriginalJNINativeMethodFunc_t *getOriginalJNINativeMethodFunc;
};

// ---------------------------------------------------------

struct RiruInit {

    int version;
    uint32_t token;
    RiruModule *module;
    RiruFuncs *funcs;
};

typedef void (RiruInit_t)(RiruInit *);

#ifdef __cplusplus
}
#endif

#endif //RIRU_H
