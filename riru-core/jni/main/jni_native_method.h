#ifndef _JNI_NATIVE_METHOD_H
#define _JNI_NATIVE_METHOD_H

#include <jni.h>

void set_nativeForkAndSpecialize(void *addr);

void set_nativeSpecializeAppProcess(void *addr);

void set_nativeForkSystemServer(void *addr);

void set_SystemProperties_set(void *addr);

const static char *nativeForkAndSpecialize_marshmallow_sig = "(II[II[[IILjava/lang/String;Ljava/lang/String;[ILjava/lang/String;Ljava/lang/String;)I";

typedef jint (*nativeForkAndSpecialize_marshmallow_t)(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jintArray, jstring, jstring);

jint nativeForkAndSpecialize_marshmallow(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jstring instructionSet, jstring appDataDir);

const static char *nativeForkAndSpecialize_oreo_sig = "(II[II[[IILjava/lang/String;Ljava/lang/String;[I[ILjava/lang/String;Ljava/lang/String;)I";

typedef jint (*nativeForkAndSpecialize_oreo_t)(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jintArray, jintArray, jstring, jstring);

jint nativeForkAndSpecialize_oreo(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jintArray fdsToIgnore, jstring instructionSet, jstring appDataDir);

const static char *nativeForkAndSpecialize_p_sig = "(II[II[[IILjava/lang/String;Ljava/lang/String;[I[IZLjava/lang/String;Ljava/lang/String;)I";

typedef jint (*nativeForkAndSpecialize_p_t)(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jintArray, jintArray, jboolean, jstring, jstring);

jint nativeForkAndSpecialize_p(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtime_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jintArray fdsToIgnore, jboolean is_child_zygote,
        jstring instructionSet, jstring appDataDir);

// removed from beta5
const static char *nativeForkAndSpecialize_q_beta4_sig = "(II[II[[IILjava/lang/String;Ljava/lang/String;[I[IZLjava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;)I";

typedef jint (*nativeForkAndSpecialize_q_beta4_t)(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jintArray, jintArray, jboolean, jstring, jstring, jstring, jobjectArray, jstring);

jint nativeForkAndSpecialize_q_beta4(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtime_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jintArray fdsToIgnore, jboolean is_child_zygote,
        jstring instructionSet, jstring appDataDir, jstring packageName,
        jobjectArray packagesForUID, jstring sandboxId);

const static char *nativeForkAndSpecialize_samsung_p_sig = "(II[II[[IILjava/lang/String;IILjava/lang/String;[I[IZLjava/lang/String;Ljava/lang/String;)I";

typedef jint (*nativeForkAndSpecialize_samsung_p_t)(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jint, jint,
        jstring, jintArray, jintArray, jboolean, jstring, jstring);

jint nativeForkAndSpecialize_samsung_p(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtime_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jint category, jint accessInfo,
        jstring se_name, jintArray fdsToClose, jintArray fdsToIgnore, jboolean is_child_zygote,
        jstring instructionSet, jstring appDataDir);

const static char *nativeForkAndSpecialize_samsung_o_sig = "(II[II[[IILjava/lang/String;IILjava/lang/String;[I[ILjava/lang/String;Ljava/lang/String;)I";

typedef jint (*nativeForkAndSpecialize_samsung_o_t)(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jint, jint,
        jstring, jintArray, jintArray, jstring, jstring);

jint nativeForkAndSpecialize_samsung_o(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jint category, jint accessInfo,
        jstring se_name, jintArray fdsToClose, jintArray fdsToIgnore, jstring instructionSet,
        jstring appDataDir);

const static char *nativeForkAndSpecialize_samsung_n_sig = "(II[II[[IILjava/lang/String;IILjava/lang/String;[ILjava/lang/String;Ljava/lang/String;I)I";

typedef jint (*nativeForkAndSpecialize_samsung_n_t)(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jint, jint,
        jstring, jintArray, jstring, jstring, jint);

jint nativeForkAndSpecialize_samsung_n(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jint category, jint accessInfo,
        jstring se_name, jintArray fdsToClose, jstring instructionSet, jstring appDataDir, jint);

const static char *nativeForkAndSpecialize_samsung_m_sig = "(II[II[[IILjava/lang/String;IILjava/lang/String;[ILjava/lang/String;Ljava/lang/String;)I";

typedef jint (*nativeForkAndSpecialize_samsung_m_t)(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jint, jint,
        jstring, jintArray, jstring, jstring);

jint nativeForkAndSpecialize_samsung_m(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jint category, jint accessInfo,
        jstring se_name, jintArray fdsToClose, jstring instructionSet, jstring appDataDir);

// -----------------------------------------------------------------

// removed from beta5
const static char *nativeSpecializeAppProcess_sig_q_beta4 = "(II[II[[IILjava/lang/String;Ljava/lang/String;ZLjava/lang/String;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;)V";
typedef void (*nativeSpecializeAppProcess_q_beta4_t)(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jboolean, jstring, jstring, jstring, jobjectArray, jstring);

void nativeSpecializeAppProcess_q_beta4(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtimeFlags,
        jobjectArray rlimits, jint mountExternal, jstring seInfo, jstring niceName,
        jboolean startChildZygote, jstring instructionSet, jstring appDataDir, jstring packageName,
        jobjectArray packagesForUID, jstring sandboxId);

const static char *nativeSpecializeAppProcess_sig_q = "(II[II[[IILjava/lang/String;Ljava/lang/String;ZLjava/lang/String;Ljava/lang/String;)V";
typedef void (*nativeSpecializeAppProcess_t)(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jboolean, jstring, jstring);

void nativeSpecializeAppProcess_q(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtimeFlags,
        jobjectArray rlimits, jint mountExternal, jstring seInfo, jstring niceName,
        jboolean startChildZygote, jstring instructionSet, jstring appDataDir);

// -----------------------------------------------------------------

const static char *nativeForkSystemServer_sig = "(II[II[[IJJ)I";

typedef jint (*nativeForkSystemServer_t)(
        JNIEnv *, jclass, uid_t, gid_t, jintArray, jint, jobjectArray, jlong, jlong);

jint nativeForkSystemServer(
        JNIEnv *env, jclass, uid_t uid, gid_t gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jlong permittedCapabilities, jlong effectiveCapabilities);

typedef jint (*SystemProperties_set_t)(JNIEnv *, jobject, jstring, jstring);

void SystemProperties_set(JNIEnv *env, jobject clazz, jstring keyJ, jstring valJ);

#endif // _JNI_NATIVE_METHOD_H
