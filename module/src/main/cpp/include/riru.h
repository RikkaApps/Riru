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
} RiruModuleInfoV9;

// ---------------------------------------------------------

typedef void *(RiruGetFunc_v9)(uint32_t token, const char *name);

typedef void (RiruSetFunc_v9)(uint32_t token, const char *name, void *func);

typedef void *(RiruGetJNINativeMethodFunc_v9)(uint32_t token, const char *className, const char *name, const char *signature);

typedef void (RiruSetJNINativeMethodFunc_v9)(uint32_t token, const char *className, const char *name, const char *signature, void *func);

typedef const JNINativeMethod *(RiruGetOriginalJNINativeMethodFunc_v9)(const char *className, const char *name, const char *signature);

typedef struct {
    RiruGetFunc_v9 *getFunc;
    RiruGetJNINativeMethodFunc_v9 *getJNINativeMethodFunc;
    RiruSetFunc_v9 *setFunc;
    RiruSetJNINativeMethodFunc_v9 *setJNINativeMethodFunc;
    RiruGetOriginalJNINativeMethodFunc_v9 *getOriginalJNINativeMethodFunc;
} RiruFuncsV9;

// ---------------------------------------------------------

typedef struct {

    uint32_t token;
    RiruFuncsV9 *funcs;
    RiruModuleInfoV9 *module;
} RiruV9;

typedef void *(RiruInit_t)(void *);

#ifdef RIRU_MODULE
#define RIRU_EXPORT __attribute__((visibility("default"))) __attribute__((used))

/**
 * Under API 9, init will be called twice.
 *
 * First, arg is a int pointer to Riru's API version, the return value should be a int pointer to the module's API version.
 *
 *
 */
void* init(void *arg) RIRU_EXPORT;

extern RiruV9 *riru_v9;
#endif

#ifdef __cplusplus
}
#endif

#endif //RIRU_H