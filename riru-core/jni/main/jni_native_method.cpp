#include <string>
#include <vector>
#include <unistd.h>
#include <mntent.h>

#include "jni_native_method.h"
#include "logging.h"
#include "misc.h"
#include "module.h"
#include "api.h"
#include "main.h"

static void *_nativeForkAndSpecialize = nullptr;
static void *_nativeSpecializeAppProcess = nullptr;
static void *_nativeForkSystemServer = nullptr;
static void *_SystemProperties_set = nullptr;

void set_nativeForkAndSpecialize(void *addr) {
    _nativeForkAndSpecialize = addr;
}

void set_nativeSpecializeAppProcess(void *addr) {
    _nativeSpecializeAppProcess = addr;
}

void set_nativeForkSystemServer(void *addr) {
    _nativeForkSystemServer = addr;
}

void set_SystemProperties_set(void *addr) {
    _SystemProperties_set = addr;
}

static int shouldSkipUid(int uid) {
    int appId = uid % 100000;

    // limit only regular app, or strange situation will happen, such as zygote process not start (dead for no reason and leave no clues?)
    // https://android.googlesource.com/platform/frameworks/base/+/android-9.0.0_r8/core/java/android/os/UserHandle.java#151
    if (appId >= 10000 && appId <= 19999) return 0;
    return 1;
}

int nativeForkAndSpecialize_calls_count = 0;

int riru_get_nativeForkAndSpecialize_calls_count() {
    return nativeForkAndSpecialize_calls_count;
}

int nativeForkSystemServer_calls_count = 0;

int riru_get_nativeForkSystemServer_calls_count() {
    return nativeForkSystemServer_calls_count;
}

int nativeSpecializeAppProcess_calls_count = 0;

int riru_get_nativeSpecializeAppProcess_calls_count() {
    return nativeSpecializeAppProcess_calls_count;
}

// -----------------------------------------------------------------

static void nativeForkAndSpecialize_pre(
        JNIEnv *env, jclass clazz, jint &uid, jint &gid, jintArray &gids, jint &runtime_flags,
        jobjectArray &rlimits, jint &mount_external, jstring &se_info, jstring &se_name,
        jintArray &fdsToClose, jintArray &fdsToIgnore, jboolean &is_child_zygote,
        jstring &instructionSet, jstring &appDataDir, jstring &packageName,
        jobjectArray &packagesForUID, jstring &sandboxId) {

    nativeForkAndSpecialize_calls_count++;

    for (auto module : *get_modules()) {
        if (!module->forkAndSpecializePre)
            continue;

        if (module->shouldSkipUid && ((shouldSkipUid_t *) module->shouldSkipUid)(uid))
            continue;

        if (!module->shouldSkipUid && shouldSkipUid(uid))
            continue;

        if (module->apiVersion >= 3) {
            ((nativeForkAndSpecialize_pre_v3_t *) module->forkAndSpecializePre)(
                    env, clazz, &uid, &gid, &gids, &runtime_flags, &rlimits, &mount_external,
                    &se_info, &se_name, &fdsToClose, &fdsToIgnore, &is_child_zygote,
                    &instructionSet, &appDataDir, &packageName, &packagesForUID, &sandboxId);
        } else if (module->apiVersion == 2) {
            ((nativeForkAndSpecialize_pre_v2_t *) module->forkAndSpecializePre)(
                    env, clazz, &uid, &gid, &gids, &runtime_flags, &rlimits, &mount_external,
                    &se_info, &se_name, &fdsToClose, &fdsToIgnore, &is_child_zygote,
                    &instructionSet, &appDataDir);
        } else {
            ((nativeForkAndSpecialize_pre_t *) module->forkAndSpecializePre)(
                    env, clazz, uid, gid, gids, runtime_flags, rlimits, mount_external, se_info,
                    se_name, fdsToClose, fdsToIgnore, is_child_zygote, instructionSet, appDataDir);
        }
    }
}

static void nativeForkAndSpecialize_post(JNIEnv *env, jclass clazz, jint uid, jint res) {

    if (res == 0) unhook_jniRegisterNativeMethods();

    for (auto module : *get_modules()) {
        if (!module->forkAndSpecializePost)
            continue;

        if (module->shouldSkipUid && ((shouldSkipUid_t *) module->shouldSkipUid)(uid))
            continue;

        if (!module->shouldSkipUid && shouldSkipUid(uid))
            continue;

        /*
         * Magic problem:
         * There is very low change that zygote process stop working and some processes forked from zygote
         * become zombie process.
         * When the problem happens:
         * The following log (%s: forkAndSpecializePost) is not printed
         * strace zygote: futex(0x6265a70698, FUTEX_WAIT_BITSET_PRIVATE, 2, NULL, 0xffffffff
         * zygote maps: 6265a70000-6265a71000 rw-p 00020000 103:04 1160  /system/lib64/liblog.so
         * 6265a70698-6265a70000+20000 is nothing in liblog
         *
         * Don't known why, so we just don't print log in zygote and see what will happen
         */
        if (res == 0) LOGV("%s: forkAndSpecializePost", module->name);
        ((nativeForkAndSpecialize_post_t *) module->forkAndSpecializePost)(env, clazz, res);
    }
}

// -----------------------------------------------------------------

static void nativeSpecializeAppProcess_pre(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtimeFlags,
        jobjectArray rlimits, jint mountExternal, jstring seInfo, jstring niceName,
        jboolean startChildZygote, jstring instructionSet, jstring appDataDir, jstring packageName,
        jobjectArray packagesForUID, jstring sandboxId) {

    nativeSpecializeAppProcess_calls_count++;

    for (auto module : *get_modules()) {
        if (!module->specializeAppProcessPre)
            continue;

        if (module->apiVersion >= 4) {
            ((nativeSpecializeAppProcess_pre_t *) module->specializeAppProcessPre)(
                    env, clazz, &uid, &gid, &gids, &runtimeFlags, &rlimits, &mountExternal, &seInfo,
                    &niceName, &startChildZygote, &instructionSet, &appDataDir, &packageName,
                    &packagesForUID, &sandboxId);
        }
    }
}

static void nativeSpecializeAppProcess_post(JNIEnv *env, jclass clazz) {

    unhook_jniRegisterNativeMethods();

    for (auto module : *get_modules()) {
        if (!module->specializeAppProcessPost)
            continue;

        LOGV("%s: specializeAppProcessPost", module->name);
        if (module->apiVersion >= 4) {
            ((nativeSpecializeAppProcess_post_t *) module->specializeAppProcessPost)(env, clazz);
        }
    }
}

// -----------------------------------------------------------------

static void nativeForkSystemServer_pre(
        JNIEnv *env, jclass clazz, uid_t &uid, gid_t &gid, jintArray &gids, jint &debug_flags,
        jobjectArray &rlimits, jlong &permittedCapabilities, jlong &effectiveCapabilities) {

    nativeForkSystemServer_calls_count++;

    for (auto module : *get_modules()) {
        if (!module->forkSystemServerPre)
            continue;

        if (module->apiVersion >= 2) {
            ((nativeForkSystemServer_pre_v2_t *) module->forkSystemServerPre)(
                    env, clazz, &uid, &gid, &gids, &debug_flags, &rlimits, &permittedCapabilities,
                    &effectiveCapabilities);
        } else {
            ((nativeForkSystemServer_pre_t *) module->forkSystemServerPre)(
                    env, clazz, uid, gid, gids, debug_flags, rlimits, permittedCapabilities,
                    effectiveCapabilities);
        }
    }
}

static void nativeForkSystemServer_post(JNIEnv *env, jclass clazz, jint res) {
    for (auto module : *get_modules()) {
        if (!module->forkSystemServerPost)
            continue;

        if (res == 0) LOGV("%s: forkSystemServerPost", module->name);
        ((nativeForkSystemServer_post_t *) module->forkSystemServerPost)(env, clazz, res);
    }
}

// -----------------------------------------------------------------

jint nativeForkAndSpecialize_marshmallow(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jstring instructionSet, jstring appDataDir) {

    jintArray fdsToIgnore = nullptr;
    jboolean is_child_zygote = JNI_FALSE;
    jstring packageName = nullptr;
    jobjectArray packagesForUID = nullptr;
    jstring sandboxId = nullptr;

    nativeForkAndSpecialize_pre(env, clazz, uid, gid, gids, debug_flags, rlimits, mount_external,
                                se_info, se_name, fdsToClose, fdsToIgnore, is_child_zygote,
                                instructionSet, appDataDir, packageName, packagesForUID,
                                sandboxId);

    jint res = ((nativeForkAndSpecialize_marshmallow_t) _nativeForkAndSpecialize)(
            env, clazz, uid, gid, gids, debug_flags, rlimits, mount_external, se_info, se_name,
            fdsToClose, instructionSet, appDataDir);

    nativeForkAndSpecialize_post(env, clazz, uid, res);
    return res;
}

jint nativeForkAndSpecialize_oreo(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jintArray fdsToIgnore, jstring instructionSet, jstring appDataDir) {

    jboolean is_child_zygote = JNI_FALSE;
    jstring packageName = nullptr;
    jobjectArray packagesForUID = nullptr;
    jstring sandboxId = nullptr;

    nativeForkAndSpecialize_pre(env, clazz, uid, gid, gids, debug_flags, rlimits, mount_external,
                                se_info, se_name, fdsToClose, fdsToIgnore, is_child_zygote,
                                instructionSet, appDataDir, packageName, packagesForUID,
                                sandboxId);

    jint res = ((nativeForkAndSpecialize_oreo_t) _nativeForkAndSpecialize)(
            env, clazz, uid, gid, gids, debug_flags, rlimits, mount_external, se_info, se_name,
            fdsToClose, fdsToIgnore, instructionSet, appDataDir);

    nativeForkAndSpecialize_post(env, clazz, uid, res);
    return res;
}

jint nativeForkAndSpecialize_p(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtime_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jintArray fdsToIgnore, jboolean is_child_zygote,
        jstring instructionSet, jstring appDataDir) {

    jstring packageName = nullptr;
    jobjectArray packagesForUID = nullptr;
    jstring sandboxId = nullptr;

    nativeForkAndSpecialize_pre(env, clazz, uid, gid, gids, runtime_flags, rlimits, mount_external,
                                se_info, se_name, fdsToClose, fdsToIgnore, is_child_zygote,
                                instructionSet, appDataDir, packageName, packagesForUID,
                                sandboxId);

    jint res = ((nativeForkAndSpecialize_p_t) _nativeForkAndSpecialize)(
            env, clazz, uid, gid, gids, runtime_flags, rlimits, mount_external, se_info, se_name,
            fdsToClose, fdsToIgnore, is_child_zygote, instructionSet, appDataDir);

    nativeForkAndSpecialize_post(env, clazz, uid, res);
    return res;
}

jint nativeForkAndSpecialize_q_beta4(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtime_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jintArray fdsToIgnore, jboolean is_child_zygote,
        jstring instructionSet, jstring appDataDir, jstring packageName,
        jobjectArray packagesForUID, jstring sandboxId) {

    nativeForkAndSpecialize_pre(env, clazz, uid, gid, gids, runtime_flags, rlimits, mount_external,
                                se_info, se_name, fdsToClose, fdsToIgnore, is_child_zygote,
                                instructionSet, appDataDir, packageName, packagesForUID,
                                sandboxId);

    jint res = ((nativeForkAndSpecialize_q_beta4_t) _nativeForkAndSpecialize)(
            env, clazz, uid, gid, gids, runtime_flags, rlimits, mount_external, se_info, se_name,
            fdsToClose, fdsToIgnore, is_child_zygote, instructionSet, appDataDir, packageName,
            packagesForUID, sandboxId);

    nativeForkAndSpecialize_post(env, clazz, uid, res);
    return res;
}

jint nativeForkAndSpecialize_samsung_p(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtime_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jint category, jint accessInfo,
        jstring se_name, jintArray fdsToClose, jintArray fdsToIgnore, jboolean is_child_zygote,
        jstring instructionSet, jstring appDataDir) {

    jstring packageName = nullptr;
    jobjectArray packagesForUID = nullptr;
    jstring sandboxId = nullptr;

    nativeForkAndSpecialize_pre(env, clazz, uid, gid, gids, runtime_flags, rlimits, mount_external,
                                se_info, se_name, fdsToClose, fdsToIgnore, is_child_zygote,
                                instructionSet, appDataDir, packageName, packagesForUID,
                                sandboxId);

    jint res = ((nativeForkAndSpecialize_samsung_p_t) _nativeForkAndSpecialize)(
            env, clazz, uid, gid, gids, runtime_flags, rlimits, mount_external, se_info, category,
            accessInfo, se_name, fdsToClose, fdsToIgnore, is_child_zygote, instructionSet,
            appDataDir);

    nativeForkAndSpecialize_post(env, clazz, uid, res);
    return res;
}

jint nativeForkAndSpecialize_samsung_o(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jint category, jint accessInfo,
        jstring se_name, jintArray fdsToClose, jintArray fdsToIgnore, jstring instructionSet,
        jstring appDataDir) {

    jboolean is_child_zygote = JNI_FALSE;
    jstring packageName = nullptr;
    jobjectArray packagesForUID = nullptr;
    jstring sandboxId = nullptr;

    nativeForkAndSpecialize_pre(env, clazz, uid, gid, gids, debug_flags, rlimits, mount_external,
                                se_info, se_name, fdsToClose, fdsToIgnore, is_child_zygote,
                                instructionSet, appDataDir, packageName, packagesForUID,
                                sandboxId);

    jint res = ((nativeForkAndSpecialize_samsung_o_t) _nativeForkAndSpecialize)(
            env, clazz, uid, gid, gids, debug_flags, rlimits, mount_external, se_info, category,
            accessInfo, se_name, fdsToClose, fdsToIgnore, instructionSet, appDataDir);

    nativeForkAndSpecialize_post(env, clazz, uid, res);
    return res;
}

jint nativeForkAndSpecialize_samsung_n(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jint category, jint accessInfo,
        jstring se_name, jintArray fdsToClose, jstring instructionSet, jstring appDataDir,
        jint a1) {

    jintArray fdsToIgnore = nullptr;
    jboolean is_child_zygote = JNI_FALSE;
    jstring packageName = nullptr;
    jobjectArray packagesForUID = nullptr;
    jstring sandboxId = nullptr;

    nativeForkAndSpecialize_pre(env, clazz, uid, gid, gids, debug_flags, rlimits, mount_external,
                                se_info, se_name, fdsToClose, fdsToIgnore, is_child_zygote,
                                instructionSet, appDataDir, packageName, packagesForUID,
                                sandboxId);

    jint res = ((nativeForkAndSpecialize_samsung_n_t) _nativeForkAndSpecialize)(
            env, clazz, uid, gid, gids, debug_flags, rlimits, mount_external, se_info, category,
            accessInfo, se_name, fdsToClose, instructionSet, appDataDir, a1);

    nativeForkAndSpecialize_post(env, clazz, uid, res);
    return res;
}

jint nativeForkAndSpecialize_samsung_m(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jint category, jint accessInfo,
        jstring se_name, jintArray fdsToClose, jstring instructionSet, jstring appDataDir) {

    jintArray fdsToIgnore = nullptr;
    jboolean is_child_zygote = JNI_FALSE;
    jstring packageName = nullptr;
    jobjectArray packagesForUID = nullptr;
    jstring sandboxId = nullptr;

    nativeForkAndSpecialize_pre(env, clazz, uid, gid, gids, debug_flags, rlimits, mount_external,
                                se_info, se_name, fdsToClose, fdsToIgnore, is_child_zygote,
                                instructionSet, appDataDir, packageName, packagesForUID,
                                sandboxId);

    jint res = ((nativeForkAndSpecialize_samsung_m_t) _nativeForkAndSpecialize)(
            env, clazz, uid, gid, gids, debug_flags, rlimits, mount_external, se_info, category,
            accessInfo, se_name, fdsToClose, instructionSet, appDataDir);

    nativeForkAndSpecialize_post(env, clazz, uid, res);
    return res;
}

// -----------------------------------------------------------------

void nativeSpecializeAppProcess_q_beta4(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtimeFlags,
        jobjectArray rlimits, jint mountExternal, jstring seInfo, jstring niceName,
        jboolean startChildZygote, jstring instructionSet, jstring appDataDir, jstring packageName,
        jobjectArray packagesForUID, jstring sandboxId) {

    nativeSpecializeAppProcess_pre(
            env, clazz, uid, gid, gids, runtimeFlags, rlimits, mountExternal, seInfo, niceName,
            startChildZygote, instructionSet, appDataDir, packageName, packagesForUID,
            sandboxId);

    ((nativeSpecializeAppProcess_q_beta4_t) _nativeSpecializeAppProcess)(
            env, clazz, uid, gid, gids, runtimeFlags, rlimits, mountExternal, seInfo, niceName,
            startChildZygote, instructionSet, appDataDir, packageName, packagesForUID,
            sandboxId);

    nativeSpecializeAppProcess_post(env, clazz);
}

void nativeSpecializeAppProcess_q(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtimeFlags,
        jobjectArray rlimits, jint mountExternal, jstring seInfo, jstring niceName,
        jboolean startChildZygote, jstring instructionSet, jstring appDataDir) {

    jstring packageName = nullptr;
    jobjectArray packagesForUID = nullptr;
    jstring sandboxId = nullptr;

    nativeSpecializeAppProcess_pre(
            env, clazz, uid, gid, gids, runtimeFlags, rlimits, mountExternal, seInfo, niceName,
            startChildZygote, instructionSet, appDataDir, packageName, packagesForUID, sandboxId);

    ((nativeSpecializeAppProcess_t) _nativeSpecializeAppProcess)(
            env, clazz, uid, gid, gids, runtimeFlags, rlimits, mountExternal, seInfo, niceName,
            startChildZygote, instructionSet, appDataDir);

    nativeSpecializeAppProcess_post(env, clazz);
}

// -----------------------------------------------------------------

jint nativeForkSystemServer(
        JNIEnv *env, jclass clazz, uid_t uid, gid_t gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jlong permittedCapabilities, jlong effectiveCapabilities) {

    nativeForkSystemServer_pre(
            env, clazz, uid, gid, gids, debug_flags, rlimits, permittedCapabilities,
            effectiveCapabilities);

    jint res = ((nativeForkSystemServer_t) _nativeForkSystemServer)(
            env, clazz, uid, gid, gids, debug_flags, rlimits, permittedCapabilities,
            effectiveCapabilities);

    nativeForkSystemServer_post(env, clazz, res);
    return res;
}

/*
 * On Android 9+, in very rare cases, SystemProperties.set("sys.user." + userId + ".ce_available", "true")
 * will throw an exception (we don't known if this is caused by Riru) and user data will be wiped.
 * So we hook it and clear the exception to prevent this problem from happening.
 *
 * log:
 * UserDataPreparer: Setting property: sys.user.0.ce_available=true
 * PackageManager: Destroying user 0 on volume null because we failed to prepare: java.lang.RuntimeException: failed to set system property
 *
 * http://androidxref.com/9.0.0_r3/xref/frameworks/base/services/core/java/com/android/server/pm/UserDataPreparer.java#107
 * -> http://androidxref.com/9.0.0_r3/xref/frameworks/base/services/core/java/com/android/server/pm/UserDataPreparer.java#112
 * -> http://androidxref.com/9.0.0_r3/xref/system/vold/VoldNativeService.cpp#751
 * -> http://androidxref.com/9.0.0_r3/xref/system/vold/Ext4Crypt.cpp#743
 * -> http://androidxref.com/9.0.0_r3/xref/system/vold/Ext4Crypt.cpp#221
 */
void SystemProperties_set(JNIEnv *env, jobject clazz, jstring keyJ, jstring valJ) {
    const char *key = env->GetStringUTFChars(keyJ, JNI_FALSE);
    char user[16];
    int no_throw = sscanf(key, "sys.user.%[^.].ce_available", user) == 1;
    env->ReleaseStringUTFChars(keyJ, key);

    ((SystemProperties_set_t) _SystemProperties_set)(env, clazz, keyJ, valJ);

    jthrowable exception = env->ExceptionOccurred();
    if (exception && no_throw) {
        LOGW("prevented data destroy");

        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}