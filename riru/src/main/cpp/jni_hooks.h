#ifndef _JNI_NATIVE_METHOD_H
#define _JNI_NATIVE_METHOD_H

#include <jni.h>
#include <riru.h>

namespace jni {

    namespace zygote {
        extern const char* classname;
        extern JNINativeMethod *nativeForkAndSpecialize;
        extern JNINativeMethod *nativeSpecializeAppProcess;
        extern JNINativeMethod *nativeForkSystemServer;
    }

    void InstallHooks();

    void RestoreHooks(JNIEnv *env);
}

const static char *nativeForkAndSpecialize_marshmallow_sig = "(II[II[[IILjava/lang/String;Ljava/lang/String;[ILjava/lang/String;Ljava/lang/String;)I";

using nativeForkAndSpecialize_marshmallow_t = jint(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jintArray, jstring, jstring);

[[clang::no_stack_protector]] jint nativeForkAndSpecialize_marshmallow(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jstring instructionSet, jstring appDataDir);

const static char *nativeForkAndSpecialize_oreo_sig = "(II[II[[IILjava/lang/String;Ljava/lang/String;[I[ILjava/lang/String;Ljava/lang/String;)I";

using nativeForkAndSpecialize_oreo_t = jint(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jintArray, jintArray, jstring, jstring);

[[clang::no_stack_protector]] jint nativeForkAndSpecialize_oreo(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jintArray fdsToIgnore, jstring instructionSet, jstring appDataDir);

const static char *nativeForkAndSpecialize_p_sig = "(II[II[[IILjava/lang/String;Ljava/lang/String;[I[IZLjava/lang/String;Ljava/lang/String;)I";

using nativeForkAndSpecialize_p_t = jint(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jintArray, jintArray, jboolean, jstring, jstring);

[[clang::no_stack_protector]] jint nativeForkAndSpecialize_p(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtime_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jintArray fdsToIgnore, jboolean is_child_zygote,
        jstring instructionSet, jstring appDataDir);

const static char *nativeForkAndSpecialize_q_alternative_sig = "(II[II[[IILjava/lang/String;Ljava/lang/String;[I[IZLjava/lang/String;Ljava/lang/String;Z)I";

using nativeForkAndSpecialize_q_alternative_t = jint(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jintArray, jintArray, jboolean, jstring, jstring, jboolean);

[[clang::no_stack_protector]] jint nativeForkAndSpecialize_q_alternative(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtime_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jintArray fdsToIgnore, jboolean is_child_zygote,
        jstring instructionSet, jstring appDataDir, jboolean isTopApp);

const static char *nativeForkAndSpecialize_r_sig = "(II[II[[IILjava/lang/String;Ljava/lang/String;[I[IZLjava/lang/String;Ljava/lang/String;Z[Ljava/lang/String;[Ljava/lang/String;ZZ)I";

using nativeForkAndSpecialize_r_t = jint(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jintArray, jintArray, jboolean, jstring, jstring, jboolean, jobjectArray, jobjectArray,
        jboolean, jboolean);

[[clang::no_stack_protector]] jint nativeForkAndSpecialize_r(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtime_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jintArray fdsToIgnore, jboolean is_child_zygote,
        jstring instructionSet, jstring appDataDir, jboolean isTopApp, jobjectArray pkgDataInfoList,
        jobjectArray whitelistedDataInfoList, jboolean bindMountAppDataDirs, jboolean bindMountAppStorageDirs);

const static char *nativeForkAndSpecialize_r_dp2_sig = "(II[II[[IILjava/lang/String;Ljava/lang/String;[I[IZLjava/lang/String;Ljava/lang/String;Z[Ljava/lang/String;)I";

using nativeForkAndSpecialize_r_dp2_t = jint(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jintArray, jintArray, jboolean, jstring, jstring, jboolean, jobjectArray);

[[clang::no_stack_protector]] jint nativeForkAndSpecialize_r_dp2(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtime_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jintArray fdsToIgnore, jboolean is_child_zygote,
        jstring instructionSet, jstring appDataDir, jboolean isTopApp, jobjectArray pkgDataInfoList);

const static char *nativeForkAndSpecialize_r_dp3_sig = "(II[II[[IILjava/lang/String;Ljava/lang/String;[I[IZLjava/lang/String;Ljava/lang/String;Z[Ljava/lang/String;Z)I";

using nativeForkAndSpecialize_r_dp3_t = jint(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jintArray, jintArray, jboolean, jstring, jstring, jboolean, jobjectArray, jboolean);

[[clang::no_stack_protector]] jint nativeForkAndSpecialize_r_dp3(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtime_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jintArray fdsToIgnore, jboolean is_child_zygote,
        jstring instructionSet, jstring appDataDir, jboolean isTopApp, jobjectArray pkgDataInfoList,
        jboolean bindMountAppStorageDirs);

const static char *nativeForkAndSpecialize_samsung_p_sig = "(II[II[[IILjava/lang/String;IILjava/lang/String;[I[IZLjava/lang/String;Ljava/lang/String;)I";

using nativeForkAndSpecialize_samsung_p_t = jint(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jint, jint,
        jstring, jintArray, jintArray, jboolean, jstring, jstring);

[[clang::no_stack_protector]] jint nativeForkAndSpecialize_samsung_p(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtime_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jint category, jint accessInfo,
        jstring se_name, jintArray fdsToClose, jintArray fdsToIgnore, jboolean is_child_zygote,
        jstring instructionSet, jstring appDataDir);

const static char *nativeForkAndSpecialize_samsung_o_sig = "(II[II[[IILjava/lang/String;IILjava/lang/String;[I[ILjava/lang/String;Ljava/lang/String;)I";

using nativeForkAndSpecialize_samsung_o_t = jint(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jint, jint,
        jstring, jintArray, jintArray, jstring, jstring);

[[clang::no_stack_protector]] jint nativeForkAndSpecialize_samsung_o(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jint category, jint accessInfo,
        jstring se_name, jintArray fdsToClose, jintArray fdsToIgnore, jstring instructionSet,
        jstring appDataDir);

const static char *nativeForkAndSpecialize_samsung_n_sig = "(II[II[[IILjava/lang/String;IILjava/lang/String;[ILjava/lang/String;Ljava/lang/String;I)I";

using nativeForkAndSpecialize_samsung_n_t = jint(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jint, jint,
        jstring, jintArray, jstring, jstring, jint);

[[clang::no_stack_protector]] jint nativeForkAndSpecialize_samsung_n(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jint category, jint accessInfo,
        jstring se_name, jintArray fdsToClose, jstring instructionSet, jstring appDataDir, jint);

const static char *nativeForkAndSpecialize_samsung_m_sig = "(II[II[[IILjava/lang/String;IILjava/lang/String;[ILjava/lang/String;Ljava/lang/String;)I";

using nativeForkAndSpecialize_samsung_m_t = jint(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jint, jint,
        jstring, jintArray, jstring, jstring);

[[clang::no_stack_protector]] jint nativeForkAndSpecialize_samsung_m(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jint category, jint accessInfo,
        jstring se_name, jintArray fdsToClose, jstring instructionSet, jstring appDataDir);

// -----------------------------------------------------------------

const static char *nativeSpecializeAppProcess_q_sig = "(II[II[[IILjava/lang/String;Ljava/lang/String;ZLjava/lang/String;Ljava/lang/String;)V";

using nativeSpecializeAppProcess_q_t = void(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jboolean, jstring, jstring);

[[clang::no_stack_protector]] void nativeSpecializeAppProcess_q(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtimeFlags,
        jobjectArray rlimits, jint mountExternal, jstring seInfo, jstring niceName,
        jboolean startChildZygote, jstring instructionSet, jstring appDataDir);

const static char *nativeSpecializeAppProcess_r_sig = "(II[II[[IILjava/lang/String;Ljava/lang/String;ZLjava/lang/String;Ljava/lang/String;Z[Ljava/lang/String;[Ljava/lang/String;ZZ)V";

using nativeSpecializeAppProcess_r_t = void(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jboolean, jstring, jstring, jboolean, jobjectArray, jobjectArray, jboolean, jboolean);

[[clang::no_stack_protector]] void nativeSpecializeAppProcess_r(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtimeFlags,
        jobjectArray rlimits, jint mountExternal, jstring seInfo, jstring niceName,
        jboolean startChildZygote, jstring instructionSet, jstring appDataDir,
        jboolean isTopApp, jobjectArray pkgDataInfoList, jobjectArray whitelistedDataInfoList,
        jboolean bindMountAppDataDirs, jboolean bindMountAppStorageDirs);

const static char *nativeSpecializeAppProcess_r_dp2_sig = "(II[II[[IILjava/lang/String;Ljava/lang/String;ZLjava/lang/String;Ljava/lang/String;Z[Ljava/lang/String;)V";

using nativeSpecializeAppProcess_r_dp2_t = void(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jboolean, jstring, jstring, jboolean, jobjectArray);

[[clang::no_stack_protector]] void nativeSpecializeAppProcess_r_dp2(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtimeFlags,
        jobjectArray rlimits, jint mountExternal, jstring seInfo, jstring niceName,
        jboolean startChildZygote, jstring instructionSet, jstring appDataDir,
        jboolean isTopApp, jobjectArray pkgDataInfoList);

const static char *nativeSpecializeAppProcess_r_dp3_sig = "(II[II[[IILjava/lang/String;Ljava/lang/String;ZLjava/lang/String;Ljava/lang/String;Z[Ljava/lang/String;Z)V";

using nativeSpecializeAppProcess_r_dp3_t = void(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jboolean, jstring, jstring, jboolean, jobjectArray, jboolean);

[[clang::no_stack_protector]] void nativeSpecializeAppProcess_r_dp3(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtimeFlags,
        jobjectArray rlimits, jint mountExternal, jstring seInfo, jstring niceName,
        jboolean startChildZygote, jstring instructionSet, jstring appDataDir,
        jboolean isTopApp, jobjectArray pkgDataInfoList, jboolean bindMountAppStorageDirs);

const static char *nativeSpecializeAppProcess_q_alternative_sig = "(II[II[[IILjava/lang/String;Ljava/lang/String;ZLjava/lang/String;Ljava/lang/String;Z)V";

using nativeSpecializeAppProcess_q_alternative_t = void(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jstring,
        jboolean, jstring, jstring, jboolean);

[[clang::no_stack_protector]] void nativeSpecializeAppProcess_q_alternative(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtimeFlags,
        jobjectArray rlimits, jint mountExternal, jstring seInfo, jstring niceName,
        jboolean startChildZygote, jstring instructionSet, jstring appDataDir,
        jboolean isTopApp);

const static char *nativeSpecializeAppProcess_sig_samsung_q = "(II[II[[IILjava/lang/String;IILjava/lang/String;ZLjava/lang/String;Ljava/lang/String;)V";

using nativeSpecializeAppProcess_samsung_t = void(
        JNIEnv *, jclass, jint, jint, jintArray, jint, jobjectArray, jint, jstring, jint,
        jint, jstring, jboolean, jstring, jstring);

[[clang::no_stack_protector]] void nativeSpecializeAppProcess_samsung_q(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtimeFlags,
        jobjectArray rlimits, jint mountExternal, jstring seInfo, jint space, jint accessInfo,
        jstring niceName, jboolean startChildZygote, jstring instructionSet, jstring appDataDir);

// -----------------------------------------------------------------

const static char *nativeForkSystemServer_sig = "(II[II[[IJJ)I";

using nativeForkSystemServer_t = jint(
        JNIEnv *, jclass, uid_t, gid_t, jintArray, jint, jobjectArray, jlong, jlong);

[[clang::no_stack_protector]] jint nativeForkSystemServer(
        JNIEnv *env, jclass, uid_t uid, gid_t gid, jintArray gids, jint runtimeFlags,
        jobjectArray rlimits, jlong permittedCapabilities, jlong effectiveCapabilities);

const static char *nativeForkSystemServer_samsung_q_sig = "(II[IIII[[IJJ)I";

using nativeForkSystemServer_samsung_q_t = jint(
        JNIEnv *, jclass, uid_t, gid_t, jintArray, jint, jint, jint, jobjectArray, jlong, jlong);

[[clang::no_stack_protector]] jint nativeForkSystemServer_samsung_q(
        JNIEnv *env, jclass, uid_t uid, gid_t gid, jintArray gids, jint runtimeFlags,
        jint space, jint accessInfo, jobjectArray rlimits, jlong permittedCapabilities,
        jlong effectiveCapabilities);

#endif // _JNI_NATIVE_METHOD_H
