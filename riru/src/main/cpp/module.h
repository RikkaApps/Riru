#pragma once

#include <jni.h>
#include <string>
#include <map>
#include <vector>
#include "api.h"

#define MODULE_NAME_CORE "core"

struct RiruModule {

public:
    const char *id;
    const char *path;
    const char *magisk_module_path;
    int apiVersion;
    uint32_t token;

    void *handle{};
    std::map<std::string, void *> *funcs;

    int supportHide;
    int version;
    const char *versionName;

private:
    void *_onModuleLoaded;
    void *_shouldSkipUid;
    void *_forkAndSpecializePre;
    void *_forkAndSpecializePost;
    void *_forkSystemServerPre;
    void *_forkSystemServerPost;
    void *_specializeAppProcessPre;
    void *_specializeAppProcessPost;

public:
    explicit RiruModule(const char *id, const char *path, const char *magisk_module_path, uint32_t token = 0) :
            id(id), path(path), magisk_module_path(magisk_module_path), token(token ? token : (uintptr_t) id) {

        funcs = new std::map<std::string, void *>();
        apiVersion = 0;
        handle = nullptr;
        _onModuleLoaded = nullptr;
        _shouldSkipUid = nullptr;
        _forkAndSpecializePre = nullptr;
        _forkAndSpecializePost = nullptr;
        _forkSystemServerPre = nullptr;
        _forkSystemServerPost = nullptr;
        _specializeAppProcessPre = nullptr;
        _specializeAppProcessPost = nullptr;
    }

    void info(RiruModuleInfo *info) {
        supportHide = info->supportHide;
        version = info->version;
        versionName = strdup(info->versionName ? info->versionName : "(null)");
        _onModuleLoaded = (void *) info->onModuleLoaded;
        _shouldSkipUid = (void *) info->shouldSkipUid;
        _forkAndSpecializePre = (void *) info->forkAndSpecializePre;
        _forkAndSpecializePost = (void *) info->forkAndSpecializePost;
        _forkSystemServerPre = (void *) info->forkSystemServerPre;
        _forkSystemServerPost = (void *) info->forkSystemServerPost;
        _specializeAppProcessPre = (void *) info->specializeAppProcessPre;
        _specializeAppProcessPost = (void *) info->specializeAppProcessPost;
    }

    bool hasOnModuleLoaded() {
        return _onModuleLoaded != nullptr;
    }

    bool hasShouldSkipUid() {
        return _shouldSkipUid != nullptr;
    }

    bool hasForkAndSpecializePre() {
        return _forkAndSpecializePre != nullptr;
    }

    bool hasForkAndSpecializePost() {
        return _forkAndSpecializePost != nullptr;
    }

    bool hasForkSystemServerPre() {
        return _forkSystemServerPre != nullptr;
    }

    bool hasForkSystemServerPost() {
        return _forkSystemServerPost != nullptr;
    }

    bool hasSpecializeAppProcessPre() {
        return _specializeAppProcessPre != nullptr;
    }

    bool hasSpecializeAppProcessPost() {
        return _specializeAppProcessPost != nullptr;
    }

    void onModuleLoaded() {
        ((onModuleLoaded_v9 *) _onModuleLoaded)();
    }

    bool shouldSkipUid(int uid) {
        return ((shouldSkipUid_v9 *) _shouldSkipUid)(uid);
    }

    void forkAndSpecializePre(
            JNIEnv *env, jclass cls, jint *uid, jint *gid, jintArray *gids, jint *runtimeFlags,
            jobjectArray *rlimits, jint *mountExternal, jstring *seInfo, jstring *niceName,
            jintArray *fdsToClose, jintArray *fdsToIgnore, jboolean *is_child_zygote,
            jstring *instructionSet, jstring *appDataDir, jboolean *isTopApp, jobjectArray *pkgDataInfoList,
            jobjectArray *whitelistedDataInfoList, jboolean *bindMountAppDataDirs, jboolean *bindMountAppStorageDirs) {

        ((nativeForkAndSpecializePre_v9 *) _forkAndSpecializePre)(
                env, cls, uid, gid, gids, runtimeFlags, rlimits, mountExternal,
                seInfo, niceName, fdsToClose, fdsToIgnore, is_child_zygote,
                instructionSet, appDataDir, isTopApp, pkgDataInfoList, whitelistedDataInfoList,
                bindMountAppDataDirs, bindMountAppStorageDirs);
    }

    void forkAndSpecializePost(JNIEnv *env, jclass cls, jint res) {
        ((nativeForkAndSpecializePost_v9 *) _forkAndSpecializePost)(
                env, cls, res);
    }

    void forkSystemServerPre(
            JNIEnv *env, jclass cls, uid_t *uid, gid_t *gid, jintArray *gids, jint *runtimeFlags,
            jobjectArray *rlimits, jlong *permittedCapabilities, jlong *effectiveCapabilities) {

        ((nativeForkSystemServerPre_v9 *) _forkSystemServerPre)(
                env, cls, uid, gid, gids, runtimeFlags, rlimits, permittedCapabilities,
                effectiveCapabilities);
    }

    void forkSystemServerPost(JNIEnv *env, jclass cls, jint res) {
        ((nativeForkSystemServerPost_v9 *) _forkSystemServerPost)(
                env, cls, res);
    }

    void specializeAppProcessPre(
            JNIEnv *env, jclass cls, jint *uid, jint *gid, jintArray *gids, jint *runtimeFlags,
            jobjectArray *rlimits, jint *mountExternal, jstring *seInfo, jstring *niceName,
            jboolean *startChildZygote, jstring *instructionSet, jstring *appDataDir,
            jboolean *isTopApp, jobjectArray *pkgDataInfoList, jobjectArray *whitelistedDataInfoList,
            jboolean *bindMountAppDataDirs, jboolean *bindMountAppStorageDirs) {

        ((nativeSpecializeAppProcessPre_v9 *) _specializeAppProcessPre)(
                env, cls, uid, gid, gids, runtimeFlags, rlimits, mountExternal, seInfo,
                niceName, startChildZygote, instructionSet, appDataDir, isTopApp,
                pkgDataInfoList, whitelistedDataInfoList, bindMountAppDataDirs, bindMountAppStorageDirs);
    }

    void specializeAppProcessPost(JNIEnv *env, jclass cls) {
        ((nativeSpecializeAppProcessPost_v9 *) _specializeAppProcessPost)(
                env, cls);
    }
};

std::vector<RiruModule *> *get_modules();

namespace Modules {
    void Load();
}

bool is_hide_enabled();