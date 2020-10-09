#ifndef RIRU_H
#define RIRU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <jni.h>
#include <sys/types.h>

// ---------------------------------------------------------

typedef void(onModuleLoaded_t)();

typedef int(shouldSkipUid_t)(int uid);

typedef void(nativeForkAndSpecializePre_t)(
        JNIEnv *env, jclass cls, jint *uid, jint *gid, jintArray *gids, jint *runtimeFlags,
        jobjectArray *rlimits, jint *mountExternal, jstring *seInfo, jstring *niceName,
        jintArray *fdsToClose, jintArray *fdsToIgnore, jboolean *is_child_zygote,
        jstring *instructionSet, jstring *appDataDir, jboolean *isTopApp, jobjectArray *pkgDataInfoList,
        jobjectArray *whitelistedDataInfoList, jboolean *bindMountAppDataDirs, jboolean *bindMountAppStorageDirs);

typedef void(nativeForkAndSpecializePost_t)(JNIEnv *env, jclass cls, jint res);

typedef void(nativeForkSystemServerPre_t)(
        JNIEnv *env, jclass cls, uid_t *uid, gid_t *gid, jintArray *gids, jint *runtimeFlags,
        jobjectArray *rlimits, jlong *permittedCapabilities, jlong *effectiveCapabilities);

typedef void(nativeForkSystemServerPost_t)(JNIEnv *env, jclass cls, jint res);

typedef void(nativeSpecializeAppProcessPre_t)(
        JNIEnv *env, jclass clazz, jint *uid, jint *gid, jintArray *gids, jint *runtimeFlags,
        jobjectArray *rlimits, jint *mountExternal, jstring *seInfo, jstring *niceName,
        jboolean *startChildZygote, jstring *instructionSet, jstring *appDataDir,
        jboolean *isTopApp, jobjectArray *pkgDataInfoList, jobjectArray *whitelistedDataInfoList,
        jboolean *bindMountAppDataDirs, jboolean *bindMountAppStorageDirs);

typedef void(nativeSpecializeAppProcessPost_t)(JNIEnv *env, jclass cls);

typedef struct {
    int apiVersion;
    int supportHide;
    onModuleLoaded_t *onModuleLoaded;
    shouldSkipUid_t *shouldSkipUid;
    nativeForkAndSpecializePre_t *forkAndSpecializePre;
    nativeForkAndSpecializePost_t *forkAndSpecializePost;
    nativeForkSystemServerPre_t *forkSystemServerPre;
    nativeForkSystemServerPost_t *forkSystemServerPost;
    nativeSpecializeAppProcessPre_t *specializeAppProcessPre;
    nativeSpecializeAppProcessPost_t *specializeAppProcessPost;
} RiruModule;

// ---------------------------------------------------------

typedef void *(RiruGetFunc_t)(uint32_t token, const char *name);

typedef void (RiruSetFunc_t)(uint32_t token, const char *name, void *func);

typedef void *(RiruGetJNINativeMethodFunc_t)(uint32_t token, const char *className, const char *name, const char *signature);

typedef void (RiruSetJNINativeMethodFunc_t)(uint32_t token, const char *className, const char *name, const char *signature, void *func);

typedef const JNINativeMethod *(RiruGetOriginalJNINativeMethodFunc_t)(const char *className, const char *name, const char *signature);

typedef struct {
    RiruGetFunc_t *getFunc;
    RiruGetJNINativeMethodFunc_t *getJNINativeMethodFunc;
    RiruSetFunc_t *setFunc;
    RiruSetJNINativeMethodFunc_t *setJNINativeMethodFunc;
    RiruGetOriginalJNINativeMethodFunc_t *getOriginalJNINativeMethodFunc;
} RiruFuncs;

// ---------------------------------------------------------

typedef struct {

    int version;
    uint32_t token;
    RiruModule *module;
    RiruFuncs *funcs;
} Riru;

typedef void (RiruInit_t)(Riru *);

#ifdef RIRU_MODULE
#define RIRU_EXPORT __attribute__((visibility("default"))) __attribute__((used))

void init(Riru *riru) RIRU_EXPORT;

extern Riru *riru;
#endif

#ifdef __cplusplus
}
#endif

#endif //RIRU_H