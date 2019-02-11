#ifndef _JNI_NATIVE_METHOD_H
#define _JNI_NATIVE_METHOD_H

#include <jni.h>

__attribute__((visibility("hidden")))
void set_nativeForkAndSpecialize(void *addr);

__attribute__((visibility("hidden")))
void set_nativeForkSystemServer(void *addr);

__attribute__((visibility("hidden")))
void set_SystemProperties_set(void *addr);

const static char *nativeForkAndSpecialize_marshmallow_sig = "(II[II[[IILjava/lang/String;Ljava/lang/String;[ILjava/lang/String;Ljava/lang/String;)I";

typedef jint (*nativeForkAndSpecialize_marshmallow_t)(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jintArray, jstring, jstring);

__attribute__((visibility("hidden")))
jint nativeForkAndSpecialize_marshmallow(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jstring instructionSet, jstring appDataDir);

const static char *nativeForkAndSpecialize_oreo_sig = "(II[II[[IILjava/lang/String;Ljava/lang/String;[I[ILjava/lang/String;Ljava/lang/String;)I";

typedef jint (*nativeForkAndSpecialize_oreo_t)(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jintArray, jintArray, jstring, jstring);

__attribute__((visibility("hidden")))
jint nativeForkAndSpecialize_oreo(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jintArray fdsToIgnore, jstring instructionSet, jstring appDataDir);

const static char *nativeForkAndSpecialize_p_sig = "(II[II[[IILjava/lang/String;Ljava/lang/String;[I[IZLjava/lang/String;Ljava/lang/String;)I";

typedef jint (*nativeForkAndSpecialize_p_t)(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jintArray, jintArray, jboolean, jstring, jstring);

__attribute__((visibility("hidden")))
jint nativeForkAndSpecialize_p(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtime_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jintArray fdsToIgnore, jboolean is_child_zygote,
        jstring instructionSet, jstring appDataDir);

const static char *nativeForkAndSpecialize_samsung_o_sig = "(II[II[[IILjava/lang/String;IILjava/lang/String;[I[ILjava/lang/String;Ljava/lang/String;)I";
typedef jint (*nativeForkAndSpecialize_samsung_o_t)(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jint, jint,
        jstring, jintArray, jintArray, jstring, jstring);

__attribute__((visibility("hidden")))
jint nativeForkAndSpecialize_samsung_o(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jint category, jint accessInfo,
        jstring se_name, jintArray fdsToClose, jintArray fdsToIgnore, jstring instructionSet,
        jstring appDataDir);

const static char *nativeForkSystemServer_sig = "(II[II[[IJJ)I";

typedef jint (*nativeForkSystemServer_t)(
        JNIEnv *, jclass, uid_t, gid_t, jintArray, jint, jobjectArray, jlong, jlong);

__attribute__((visibility("hidden")))
jint nativeForkSystemServer(
        JNIEnv *env, jclass, uid_t uid, gid_t gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jlong permittedCapabilities, jlong effectiveCapabilities);

typedef jint (*SystemProperties_set_t)(JNIEnv *, jobject, jstring, jstring);

__attribute__((visibility("hidden")))
void SystemProperties_set(JNIEnv *env, jobject clazz, jstring keyJ, jstring valJ);

#endif // _JNI_NATIVE_METHOD_H
