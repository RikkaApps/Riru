#include <string>
#include <vector>
#include <unistd.h>
#include <mntent.h>
#include <dlfcn.h>

#include "jni_native_method.h"
#include "logging.h"
#include "misc.h"
#include "init.h"
#include "module.h"

int shouldSkipUid(int uid) {
    int appId = uid % 100000;

    // limit only regular app, or strange situation will happen, such as zygote process not start (dead for no reason and leave no clues?)
    // https://android.googlesource.com/platform/frameworks/base/+/android-9.0.0_r8/core/java/android/os/UserHandle.java#151
    if (appId >= 10000 && appId <= 19999) return 0;
    return 1;
}

void nativeForkAndSpecialize_pre(JNIEnv *env, jclass clazz, jint uid, jint gid,
                                 jintArray gids,
                                 jint runtime_flags, jobjectArray rlimits,
                                 jint mount_external, jstring se_info, jstring se_name,
                                 jintArray fdsToClose, jintArray fdsToIgnore,
                                 jboolean is_child_zygote,
                                 jstring instructionSet, jstring appDataDir) {
    if (shouldSkipUid(uid))
        return;

    for (auto module : *get_modules()) {
        if (!module->forkAndSpecializePre)
            continue;

        LOGV("%s: forkAndSpecializePre", module->name);
        ((nativeForkAndSpecialize_pre_t) module->forkAndSpecializePre)(env, clazz, uid, gid,
                                                                      gids, runtime_flags,
                                                                      rlimits, mount_external,
                                                                      se_info, se_name,
                                                                      fdsToClose, fdsToIgnore,
                                                                      is_child_zygote,
                                                                      instructionSet,
                                                                      appDataDir);
    }
}

void nativeForkAndSpecialize_post(JNIEnv *env, jclass clazz, jint uid, jint res) {
    if (shouldSkipUid(uid))
        return;

    for (auto module : *get_modules()) {
        if (!module->forkAndSpecializePost)
            continue;

        LOGV("%s: forkAndSpecializePost", module->name);
        ((nativeForkAndSpecialize_post_t) module->forkAndSpecializePost)(env, clazz, res);
    }
}

void nativeForkSystemServer_pre(JNIEnv *env, jclass clazz, uid_t uid, gid_t gid, jintArray gids,
                                jint debug_flags, jobjectArray rlimits, jlong permittedCapabilities,
                                jlong effectiveCapabilities) {
    for (auto module : *get_modules()) {
        if (!module->forkSystemServerPre)
            continue;

        LOGV("%s: forkSystemServerPre", module->name);
        ((nativeForkSystemServer_pre_t) module->forkSystemServerPre)(env, clazz, uid, gid, gids,
                                                                    debug_flags, rlimits,
                                                                    permittedCapabilities,
                                                                    effectiveCapabilities);
    }
}

void nativeForkSystemServer_post(JNIEnv *env, jclass clazz, jint res) {
    for (auto module : *get_modules()) {
        if (!module->forkSystemServerPost)
            continue;

        LOGV("%s: forkSystemServerPost", module->name);
        ((nativeForkSystemServer_post_t) module->forkSystemServerPost)(env, clazz, res);
    }
}


jint nativeForkAndSpecialize_marshmallow(JNIEnv *env, jclass clazz, jint uid, jint gid,
                                         jintArray gids, jint debug_flags, jobjectArray rlimits,
                                         jint mount_external, jstring se_info, jstring se_name,
                                         jintArray fdsToClose, jstring instructionSet,
                                         jstring appDataDir) {
    nativeForkAndSpecialize_pre(env, clazz, uid, gid, gids, debug_flags, rlimits, mount_external,
                                se_info, se_name, fdsToClose, nullptr, 0, instructionSet,
                                appDataDir);

    jint res = ((nativeForkAndSpecialize_marshmallow_t) _nativeForkAndSpecialize)(env, clazz, uid,
                                                                                  gid, gids,
                                                                                  debug_flags,
                                                                                  rlimits,
                                                                                  mount_external,
                                                                                  se_info,
                                                                                  se_name,
                                                                                  fdsToClose,
                                                                                  instructionSet,
                                                                                  appDataDir);
    nativeForkAndSpecialize_post(env, clazz, uid, res);
    return res;
}

jint nativeForkAndSpecialize_oreo(JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids,
                                  jint debug_flags, jobjectArray rlimits, jint mount_external,
                                  jstring se_info, jstring se_name, jintArray fdsToClose,
                                  jintArray fdsToIgnore, jstring instructionSet,
                                  jstring appDataDir) {
    nativeForkAndSpecialize_pre(env, clazz, uid, gid, gids, debug_flags, rlimits, mount_external,
                                se_info, se_name, fdsToClose, fdsToIgnore, 0, instructionSet,
                                appDataDir);
    jint res = ((nativeForkAndSpecialize_oreo_t) _nativeForkAndSpecialize)(env, clazz, uid, gid,
                                                                           gids,
                                                                           debug_flags, rlimits,
                                                                           mount_external, se_info,
                                                                           se_name, fdsToClose,
                                                                           fdsToIgnore,
                                                                           instructionSet,
                                                                           appDataDir);
    nativeForkAndSpecialize_post(env, clazz, uid, res);
    return res;
}

jint nativeForkAndSpecialize_p(JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids,
                               jint runtime_flags, jobjectArray rlimits, jint mount_external,
                               jstring se_info, jstring se_name, jintArray fdsToClose,
                               jintArray fdsToIgnore, jboolean is_child_zygote,
                               jstring instructionSet, jstring appDataDir) {
    nativeForkAndSpecialize_pre(env, clazz, uid, gid, gids, runtime_flags, rlimits, mount_external,
                                se_info, se_name, fdsToClose, fdsToIgnore, is_child_zygote,
                                instructionSet, appDataDir);
    jint res = ((nativeForkAndSpecialize_p_t) _nativeForkAndSpecialize)(env, clazz, uid, gid, gids,
                                                                        runtime_flags, rlimits,
                                                                        mount_external, se_info,
                                                                        se_name, fdsToClose,
                                                                        fdsToIgnore,
                                                                        is_child_zygote,
                                                                        instructionSet, appDataDir);
    nativeForkAndSpecialize_post(env, clazz, uid, res);
    return res;
}

jint nativeForkSystemServer(JNIEnv *env, jclass clazz, uid_t uid, gid_t gid, jintArray gids,
                            jint debug_flags, jobjectArray rlimits, jlong permittedCapabilities,
                            jlong effectiveCapabilities) {
    nativeForkSystemServer_pre(env, clazz, uid, gid, gids, debug_flags, rlimits,
                               permittedCapabilities,
                               effectiveCapabilities);
    jint res = ((nativeForkSystemServer_t) _nativeForkSystemServer)(env, clazz, uid, gid, gids,
                                                                    debug_flags, rlimits,
                                                                    permittedCapabilities,
                                                                    effectiveCapabilities);
    nativeForkSystemServer_post(env, clazz, res);
    return res;
}