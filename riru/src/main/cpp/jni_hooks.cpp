#include <vector>
#include <unistd.h>
#include <mntent.h>
#include <android_prop.h>
#include <xhook.h>
#include <dlfcn.h>
#include <elf_util.h>

#include "jni_hooks.h"
#include "logging.h"
#include "module.h"
#include "entry.h"

namespace jni::zygote {
    const char *classname = "com/android/internal/os/Zygote";
    JNINativeMethod *nativeForkAndSpecialize = nullptr;
    JNINativeMethod *nativeSpecializeAppProcess = nullptr;
    JNINativeMethod *nativeForkSystemServer = nullptr;
}

static int shouldSkipUid(int uid) {
    int appId = uid % 100000;
    if (appId >= 10000 && appId <= 19999) return 0;
    return 1;
}

// -----------------------------------------------------------------

static bool useTableOverride = false;

using GetJniNativeInterface_t = const JNINativeInterface *();
using SetTableOverride_t = void(JNINativeInterface *);
using RegisterNatives_t = jint(JNIEnv *, jclass, const JNINativeMethod *, jint);

static SetTableOverride_t *setTableOverride = nullptr;
static RegisterNatives_t *old_RegisterNatives = nullptr;

static std::unique_ptr<JNINativeMethod[]>
onRegisterZygote(const char *className, const JNINativeMethod *methods, int numMethods) {

    auto newMethods = std::make_unique<JNINativeMethod[]>(numMethods);
    memcpy(newMethods.get(), methods, sizeof(JNINativeMethod) * numMethods);

    JNINativeMethod method;
    for (int i = 0; i < numMethods; ++i) {
        method = methods[i];

        if (strcmp(method.name, "nativeForkAndSpecialize") == 0) {
            jni::zygote::nativeForkAndSpecialize = new JNINativeMethod{method.name,
                                                                       method.signature,
                                                                       method.fnPtr};

            if (strcmp(nativeForkAndSpecialize_r_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkAndSpecialize_r;
            else if (strcmp(nativeForkAndSpecialize_p_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkAndSpecialize_p;
            else if (strcmp(nativeForkAndSpecialize_oreo_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkAndSpecialize_oreo;
            else if (strcmp(nativeForkAndSpecialize_marshmallow_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkAndSpecialize_marshmallow;

            else if (strcmp(nativeForkAndSpecialize_r_dp3_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkAndSpecialize_r_dp3;
            else if (strcmp(nativeForkAndSpecialize_r_dp2_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkAndSpecialize_r_dp2;

            else if (strcmp(nativeForkAndSpecialize_q_alternative_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkAndSpecialize_q_alternative;

            else if (strcmp(nativeForkAndSpecialize_samsung_p_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkAndSpecialize_samsung_p;
            else if (strcmp(nativeForkAndSpecialize_samsung_o_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkAndSpecialize_samsung_o;
            else if (strcmp(nativeForkAndSpecialize_samsung_n_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkAndSpecialize_samsung_n;
            else if (strcmp(nativeForkAndSpecialize_samsung_m_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkAndSpecialize_samsung_m;

            else
                LOGW("found nativeForkAndSpecialize but signature %s mismatch", method.signature);

            auto replaced = newMethods[i].fnPtr != methods[i].fnPtr;
            if (replaced) {
                LOGI("replaced com.android.internal.os.Zygote#nativeForkAndSpecialize");
            }
        } else if (strcmp(method.name, "nativeSpecializeAppProcess") == 0) {
            jni::zygote::nativeSpecializeAppProcess = new JNINativeMethod{method.name,
                                                                          method.signature,
                                                                          method.fnPtr};

            if (strcmp(nativeSpecializeAppProcess_r_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeSpecializeAppProcess_r;
            else if (strcmp(nativeSpecializeAppProcess_q_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeSpecializeAppProcess_q;
            else if (strcmp(nativeSpecializeAppProcess_q_alternative_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeSpecializeAppProcess_q_alternative;
            else if (strcmp(nativeSpecializeAppProcess_sig_samsung_q, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeSpecializeAppProcess_samsung_q;

            else if (strcmp(nativeSpecializeAppProcess_r_dp3_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeSpecializeAppProcess_r_dp3;
            else if (strcmp(nativeSpecializeAppProcess_r_dp2_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeSpecializeAppProcess_r_dp2;

            else
                LOGW("found nativeSpecializeAppProcess but signature %s mismatch",
                     method.signature);

            auto replaced = newMethods[i].fnPtr != methods[i].fnPtr;
            if (replaced) {
                LOGI("replaced com.android.internal.os.Zygote#nativeSpecializeAppProcess");
            }
        } else if (strcmp(method.name, "nativeForkSystemServer") == 0) {
            jni::zygote::nativeForkSystemServer = new JNINativeMethod{method.name, method.signature,
                                                                      method.fnPtr};

            if (strcmp(nativeForkSystemServer_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkSystemServer;
            else if (strcmp(nativeForkSystemServer_samsung_q_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkSystemServer_samsung_q;
            else
                LOGW("found nativeForkSystemServer but signature %s mismatch", method.signature);

            auto replaced = newMethods[i].fnPtr != methods[i].fnPtr;
            if (replaced) {
                LOGI("replaced com.android.internal.os.Zygote#nativeForkSystemServer");
            }
        }
    }

    return newMethods;
}

static std::unique_ptr<JNINativeMethod[]>
handleRegisterNative(const char *className, const JNINativeMethod *methods, int numMethods) {
    if (strcmp("com/android/internal/os/Zygote", className) == 0) {
        return onRegisterZygote(className, methods, numMethods);
    } else {
        return nullptr;
    }
}

#define XHOOK_REGISTER(PATH_REGEX, NAME) \
    if (xhook_register(PATH_REGEX, #NAME, (void*) new_##NAME, (void **) &old_##NAME) != 0) \
        LOGE("failed to register hook " #NAME "."); \

#define NEW_FUNC_DEF(ret, func, ...) \
    using func##_t = ret(__VA_ARGS__); \
    static func##_t *old_##func; \
    static ret new_##func(__VA_ARGS__)

NEW_FUNC_DEF(int, jniRegisterNativeMethods, JNIEnv *env, const char *className,
             const JNINativeMethod *methods, int numMethods) {
    LOGD("jniRegisterNativeMethods %s", className);

    auto newMethods = handleRegisterNative(className, methods, numMethods);
    int res = old_jniRegisterNativeMethods(env, className, newMethods ? newMethods.get() : methods,
                                           numMethods);
    /*if (!newMethods) {
        NativeMethod::jniRegisterNativeMethodsPost(env, className, methods, numMethods);
    }*/
    return res;
}

static jclass zygoteClass;

static void prepareClassesForRegisterNativeHook(JNIEnv *env) {
    static bool called = false;
    if (called) return;
    called = true;

    auto _zygoteClass = env->FindClass("com/android/internal/os/Zygote");

    // There are checks that enforces no local refs exists during Runtime::Start, make them global ref
    zygoteClass = (jclass) env->NewGlobalRef(_zygoteClass);

    env->DeleteLocalRef(_zygoteClass);
}

static int
new_RegisterNative(JNIEnv *env, jclass cls, const JNINativeMethod *methods, jint numMethods) {
    prepareClassesForRegisterNativeHook(env);

    const char *className;
    if (zygoteClass != nullptr && env->IsSameObject(zygoteClass, cls)) {
        className = "com/android/internal/os/Zygote";
        LOGD("RegisterNative %s", className);
        env->DeleteGlobalRef(zygoteClass);
        zygoteClass = nullptr;
    } else {
        className = "";
    }

    auto newMethods = handleRegisterNative(className, methods, numMethods);
    auto res = old_RegisterNatives(env, cls, newMethods ? newMethods.get() : methods, numMethods);
    return res;
}

#define RestoreJNIMethod(_cls, method) \
    if (jni::_cls::method != nullptr) { \
        if (old_jniRegisterNativeMethods) \
        old_jniRegisterNativeMethods(env, jni::_cls::classname, jni::_cls::method, 1); \
        delete jni::_cls::method; \
    }

void jni::RestoreHooks(JNIEnv *env) {
    if (useTableOverride) {
        setTableOverride(nullptr);
    } else {
        xhook_register(".*\\libandroid_runtime.so$", "jniRegisterNativeMethods",
                       (void *) old_jniRegisterNativeMethods,
                       nullptr);
        if (xhook_refresh(0) == 0) {
            xhook_clear();
        }
    }

    RestoreJNIMethod(zygote, nativeForkAndSpecialize)
    RestoreJNIMethod(zygote, nativeSpecializeAppProcess)
    RestoreJNIMethod(zygote, nativeForkSystemServer)

    LOGD("hooks restored");
}

void jni::InstallHooks() {
    XHOOK_REGISTER(".*\\libandroid_runtime.so$", jniRegisterNativeMethods)

    if (xhook_refresh(0) == 0) {
        xhook_clear();
        LOGI("hook installed");
    } else {
        LOGE("failed to refresh hook");
    }

    useTableOverride = old_jniRegisterNativeMethods == nullptr;

    if (useTableOverride) {
        LOGI("no jniRegisterNativeMethods");

        SandHook::ElfImg art("libart.so");

        auto *GetJniNativeInterface = art.getSymbAddress<GetJniNativeInterface_t *>(
                "_ZN3art21GetJniNativeInterfaceEv");
        setTableOverride = art.getSymbAddress<SetTableOverride_t *>(
                "_ZN3art9JNIEnvExt16SetTableOverrideEPK18JNINativeInterface");

        if (setTableOverride != nullptr && GetJniNativeInterface != nullptr) {
            auto functions = GetJniNativeInterface();
            auto new_JNINativeInterface = new JNINativeInterface();
            memcpy(new_JNINativeInterface, functions, sizeof(JNINativeInterface));
            old_RegisterNatives = functions->RegisterNatives;
            new_JNINativeInterface->RegisterNatives = new_RegisterNative;

            setTableOverride(new_JNINativeInterface);
            LOGI("override table installed");
        } else {
            if (GetJniNativeInterface == nullptr) LOGE("cannot find GetJniNativeInterface");
            if (setTableOverride == nullptr) LOGE("cannot find setTableOverride");
        }

        auto *handle = dlopen("libnativehelper.so", 0);
        if (handle) {
            old_jniRegisterNativeMethods = reinterpret_cast<jniRegisterNativeMethods_t *>(dlsym(
                    handle,
                    "jniRegisterNativeMethods"));
        }
    }
}

// -----------------------------------------------------------------

static void nativeForkAndSpecialize_pre(
        JNIEnv *env, jclass clazz, jint &uid, jint &gid, jintArray &gids, jint &runtime_flags,
        jobjectArray &rlimits, jint &mount_external, jstring &se_info, jstring &se_name,
        jintArray &fdsToClose, jintArray &fdsToIgnore, jboolean &is_child_zygote,
        jstring &instructionSet, jstring &appDataDir, jboolean &isTopApp,
        jobjectArray &pkgDataInfoList,
        jobjectArray &whitelistedDataInfoList, jboolean &bindMountAppDataDirs,
        jboolean &bindMountAppStorageDirs) {

    for (const auto &module : modules::Get()) {
        if (!module.hasForkAndSpecializePre())
            continue;

        module.resetAllowUnload();

        if (module.apiVersion < 25) {
            if (module.hasShouldSkipUid() && module.shouldSkipUid(uid))
                continue;

            if (!module.hasShouldSkipUid() && shouldSkipUid(uid))
                continue;
        }

        module.forkAndSpecializePre(
                env, clazz, &uid, &gid, &gids, &runtime_flags, &rlimits, &mount_external,
                &se_info, &se_name, &fdsToClose, &fdsToIgnore, &is_child_zygote,
                &instructionSet, &appDataDir, &isTopApp, &pkgDataInfoList, &whitelistedDataInfoList,
                &bindMountAppDataDirs, &bindMountAppStorageDirs);
    }
}

static void
nativeForkAndSpecialize_post(JNIEnv *env, jclass clazz, jint uid, jboolean is_child_zygote,
                             jint res) {

    if (res == 0) jni::RestoreHooks(env);

    for (const auto &module : modules::Get()) {
        if (!module.hasForkAndSpecializePost())
            continue;

        if (module.apiVersion < 25) {
            if (module.hasShouldSkipUid() && module.shouldSkipUid(uid))
                continue;

            if (!module.hasShouldSkipUid() && shouldSkipUid(uid))
                continue;
        }

        if (res == 0) LOGD("%s: forkAndSpecializePost", module.id.data());

        module.forkAndSpecializePost(env, clazz, res);
    }

    if (res == 0) Entry::Unload(is_child_zygote);
}

// -----------------------------------------------------------------

static void nativeSpecializeAppProcess_pre(
        JNIEnv *env, jclass clazz, jint &uid, jint &gid, jintArray &gids, jint &runtimeFlags,
        jobjectArray &rlimits, jint &mountExternal, jstring &seInfo, jstring &niceName,
        jboolean &startChildZygote, jstring &instructionSet, jstring &appDataDir,
        jboolean &isTopApp, jobjectArray &pkgDataInfoList, jobjectArray &whitelistedDataInfoList,
        jboolean &bindMountAppDataDirs, jboolean &bindMountAppStorageDirs) {

    for (auto &module : modules::Get()) {
        if (!module.hasSpecializeAppProcessPre())
            continue;

        module.resetAllowUnload();

        if (module.apiVersion < 25) {
            if (module.hasShouldSkipUid() && module.shouldSkipUid(uid))
                continue;

            if (!module.hasShouldSkipUid() && shouldSkipUid(uid))
                continue;
        }

        module.specializeAppProcessPre(
                env, clazz, &uid, &gid, &gids, &runtimeFlags, &rlimits, &mountExternal, &seInfo,
                &niceName, &startChildZygote, &instructionSet, &appDataDir, &isTopApp,
                &pkgDataInfoList, &whitelistedDataInfoList, &bindMountAppDataDirs,
                &bindMountAppStorageDirs);
    }
}

static void
nativeSpecializeAppProcess_post(JNIEnv *env, jclass clazz, jint uid, jboolean is_child_zygote) {

    jni::RestoreHooks(env);

    for (const auto &module : modules::Get()) {
        if (!module.hasSpecializeAppProcessPost())
            continue;

        if (module.apiVersion < 25) {
            if (module.hasShouldSkipUid() && module.shouldSkipUid(uid))
                continue;

            if (!module.hasShouldSkipUid() && shouldSkipUid(uid))
                continue;
        }

        LOGD("%s: specializeAppProcessPost", module.id.data());
        module.specializeAppProcessPost(env, clazz);
    }

    Entry::Unload(is_child_zygote);
}

// -----------------------------------------------------------------

static void nativeForkSystemServer_pre(
        JNIEnv *env, jclass clazz, uid_t &uid, gid_t &gid, jintArray &gids, jint &debug_flags,
        jobjectArray &rlimits, jlong &permittedCapabilities, jlong &effectiveCapabilities) {

    for (const auto &module : modules::Get()) {
        if (!module.hasForkSystemServerPre())
            continue;

        module.resetAllowUnload();

        module.forkSystemServerPre(
                env, clazz, &uid, &gid, &gids, &debug_flags, &rlimits, &permittedCapabilities,
                &effectiveCapabilities);
    }
}

static void nativeForkSystemServer_post(JNIEnv *env, jclass clazz, jint res) {

    if (res == 0) jni::RestoreHooks(env);

    if (res == 0 && android_prop::CheckZTE()) {
        auto *process = env->FindClass("android/os/Process");
        auto *set_argv0 = env->GetStaticMethodID(process, "setArgV0", "(Ljava/lang/String;)V");
        env->CallStaticVoidMethod(process, set_argv0, env->NewStringUTF("system_server"));
    }

    for (const auto &module : modules::Get()) {
        if (!module.hasForkSystemServerPost()) continue;

        if (res == 0) LOGD("%s: forkSystemServerPost", module.id.data());
        module.forkSystemServerPost(env, clazz, res);
    }
}

// -----------------------------------------------------------------
[[clang::no_stack_protector]]
jint nativeForkAndSpecialize_marshmallow(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jstring instructionSet, jstring appDataDir) {

    jintArray fdsToIgnore = nullptr;
    jboolean is_child_zygote = JNI_FALSE;
    jboolean isTopApp = JNI_FALSE;
    jobjectArray pkgDataInfoList = nullptr;
    jobjectArray whitelistedDataInfoList = nullptr;
    jboolean bindMountAppDataDirs = JNI_FALSE;
    jboolean bindMountAppStorageDirs = JNI_FALSE;

    nativeForkAndSpecialize_pre(env, clazz, uid, gid, gids, debug_flags, rlimits, mount_external,
                                se_info, se_name, fdsToClose, fdsToIgnore, is_child_zygote,
                                instructionSet, appDataDir, isTopApp, pkgDataInfoList,
                                whitelistedDataInfoList,
                                bindMountAppDataDirs, bindMountAppStorageDirs);

    jint res = ((nativeForkAndSpecialize_marshmallow_t *) jni::zygote::nativeForkAndSpecialize->fnPtr)(
            env, clazz, uid, gid, gids, debug_flags, rlimits, mount_external, se_info, se_name,
            fdsToClose, instructionSet, appDataDir);

    nativeForkAndSpecialize_post(env, clazz, uid, is_child_zygote, res);
    return res;
}

[[clang::no_stack_protector]]
jint nativeForkAndSpecialize_oreo(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jintArray fdsToIgnore, jstring instructionSet, jstring appDataDir) {

    jboolean is_child_zygote = JNI_FALSE;
    jboolean isTopApp = JNI_FALSE;
    jobjectArray pkgDataInfoList = nullptr;
    jobjectArray whitelistedDataInfoList = nullptr;
    jboolean bindMountAppDataDirs = JNI_FALSE;
    jboolean bindMountAppStorageDirs = JNI_FALSE;

    nativeForkAndSpecialize_pre(env, clazz, uid, gid, gids, debug_flags, rlimits, mount_external,
                                se_info, se_name, fdsToClose, fdsToIgnore, is_child_zygote,
                                instructionSet, appDataDir, isTopApp, pkgDataInfoList,
                                whitelistedDataInfoList,
                                bindMountAppDataDirs, bindMountAppStorageDirs);

    jint res = ((nativeForkAndSpecialize_oreo_t *) jni::zygote::nativeForkAndSpecialize->fnPtr)(
            env, clazz, uid, gid, gids, debug_flags, rlimits, mount_external, se_info, se_name,
            fdsToClose, fdsToIgnore, instructionSet, appDataDir);

    nativeForkAndSpecialize_post(env, clazz, uid, is_child_zygote, res);
    return res;
}

[[clang::no_stack_protector]]
jint nativeForkAndSpecialize_p(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtime_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jintArray fdsToIgnore, jboolean is_child_zygote,
        jstring instructionSet, jstring appDataDir) {

    jboolean isTopApp = JNI_FALSE;
    jobjectArray pkgDataInfoList = nullptr;
    jobjectArray whitelistedDataInfoList = nullptr;
    jboolean bindMountAppDataDirs = JNI_FALSE;
    jboolean bindMountAppStorageDirs = JNI_FALSE;

    nativeForkAndSpecialize_pre(env, clazz, uid, gid, gids, runtime_flags, rlimits, mount_external,
                                se_info, se_name, fdsToClose, fdsToIgnore, is_child_zygote,
                                instructionSet, appDataDir, isTopApp, pkgDataInfoList,
                                whitelistedDataInfoList,
                                bindMountAppDataDirs, bindMountAppStorageDirs);

    jint res = ((nativeForkAndSpecialize_p_t *) jni::zygote::nativeForkAndSpecialize->fnPtr)(
            env, clazz, uid, gid, gids, runtime_flags, rlimits, mount_external, se_info, se_name,
            fdsToClose, fdsToIgnore, is_child_zygote, instructionSet, appDataDir);

    nativeForkAndSpecialize_post(env, clazz, uid, is_child_zygote, res);
    return res;
}

[[clang::no_stack_protector]]
jint nativeForkAndSpecialize_q_alternative(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtime_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jintArray fdsToIgnore, jboolean is_child_zygote,
        jstring instructionSet, jstring appDataDir, jboolean isTopApp) {

    jobjectArray pkgDataInfoList = nullptr;
    jobjectArray whitelistedDataInfoList = nullptr;
    jboolean bindMountAppDataDirs = JNI_FALSE;
    jboolean bindMountAppStorageDirs = JNI_FALSE;

    nativeForkAndSpecialize_pre(env, clazz, uid, gid, gids, runtime_flags, rlimits, mount_external,
                                se_info, se_name, fdsToClose, fdsToIgnore, is_child_zygote,
                                instructionSet, appDataDir, isTopApp, pkgDataInfoList,
                                whitelistedDataInfoList,
                                bindMountAppDataDirs, bindMountAppStorageDirs);

    jint res = ((nativeForkAndSpecialize_q_alternative_t *) jni::zygote::nativeForkAndSpecialize->fnPtr)(
            env, clazz, uid, gid, gids, runtime_flags, rlimits, mount_external, se_info, se_name,
            fdsToClose, fdsToIgnore, is_child_zygote, instructionSet, appDataDir, isTopApp);

    nativeForkAndSpecialize_post(env, clazz, uid, is_child_zygote, res);
    return res;
}

[[clang::no_stack_protector]]
jint nativeForkAndSpecialize_r(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtime_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jintArray fdsToIgnore, jboolean is_child_zygote,
        jstring instructionSet, jstring appDataDir, jboolean isTopApp, jobjectArray pkgDataInfoList,
        jobjectArray whitelistedDataInfoList, jboolean bindMountAppDataDirs,
        jboolean bindMountAppStorageDirs) {

    nativeForkAndSpecialize_pre(env, clazz, uid, gid, gids, runtime_flags, rlimits, mount_external,
                                se_info, se_name, fdsToClose, fdsToIgnore, is_child_zygote,
                                instructionSet, appDataDir, isTopApp, pkgDataInfoList,
                                whitelistedDataInfoList,
                                bindMountAppDataDirs, bindMountAppStorageDirs);

    jint res = ((nativeForkAndSpecialize_r_t *) jni::zygote::nativeForkAndSpecialize->fnPtr)(
            env, clazz, uid, gid, gids, runtime_flags, rlimits, mount_external, se_info, se_name,
            fdsToClose, fdsToIgnore, is_child_zygote, instructionSet, appDataDir, isTopApp,
            pkgDataInfoList,
            whitelistedDataInfoList, bindMountAppDataDirs, bindMountAppStorageDirs);

    nativeForkAndSpecialize_post(env, clazz, uid, is_child_zygote, res);
    return res;
}

[[clang::no_stack_protector]]
jint nativeForkAndSpecialize_r_dp3(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtime_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jintArray fdsToIgnore, jboolean is_child_zygote,
        jstring instructionSet, jstring appDataDir, jboolean isTopApp, jobjectArray pkgDataInfoList,
        jboolean bindMountAppStorageDirs) {

    jobjectArray whitelistedDataInfoList = nullptr;
    jboolean bindMountAppDataDirs = JNI_FALSE;

    nativeForkAndSpecialize_pre(env, clazz, uid, gid, gids, runtime_flags, rlimits, mount_external,
                                se_info, se_name, fdsToClose, fdsToIgnore, is_child_zygote,
                                instructionSet, appDataDir, isTopApp, pkgDataInfoList,
                                whitelistedDataInfoList,
                                bindMountAppDataDirs, bindMountAppStorageDirs);

    jint res = ((nativeForkAndSpecialize_r_dp3_t *) jni::zygote::nativeForkAndSpecialize->fnPtr)(
            env, clazz, uid, gid, gids, runtime_flags, rlimits, mount_external, se_info, se_name,
            fdsToClose, fdsToIgnore, is_child_zygote, instructionSet, appDataDir, isTopApp,
            pkgDataInfoList,
            bindMountAppStorageDirs);

    nativeForkAndSpecialize_post(env, clazz, uid, is_child_zygote, res);
    return res;
}

[[clang::no_stack_protector]]
jint nativeForkAndSpecialize_r_dp2(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtime_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jstring se_name,
        jintArray fdsToClose, jintArray fdsToIgnore, jboolean is_child_zygote,
        jstring instructionSet, jstring appDataDir, jboolean isTopApp,
        jobjectArray pkgDataInfoList) {

    jobjectArray whitelistedDataInfoList = nullptr;
    jboolean bindMountAppDataDirs = JNI_FALSE;
    jboolean bindMountAppStorageDirs = JNI_FALSE;

    nativeForkAndSpecialize_pre(env, clazz, uid, gid, gids, runtime_flags, rlimits, mount_external,
                                se_info, se_name, fdsToClose, fdsToIgnore, is_child_zygote,
                                instructionSet, appDataDir, isTopApp, pkgDataInfoList,
                                whitelistedDataInfoList,
                                bindMountAppDataDirs, bindMountAppStorageDirs);

    jint res = ((nativeForkAndSpecialize_r_dp2_t *) jni::zygote::nativeForkAndSpecialize->fnPtr)(
            env, clazz, uid, gid, gids, runtime_flags, rlimits, mount_external, se_info, se_name,
            fdsToClose, fdsToIgnore, is_child_zygote, instructionSet, appDataDir, isTopApp,
            pkgDataInfoList);

    nativeForkAndSpecialize_post(env, clazz, uid, is_child_zygote, res);
    return res;
}

[[clang::no_stack_protector]]
jint nativeForkAndSpecialize_samsung_p(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtime_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jint category, jint accessInfo,
        jstring se_name, jintArray fdsToClose, jintArray fdsToIgnore, jboolean is_child_zygote,
        jstring instructionSet, jstring appDataDir) {

    jboolean isTopApp = JNI_FALSE;
    jobjectArray pkgDataInfoList = nullptr;
    jobjectArray whitelistedDataInfoList = nullptr;
    jboolean bindMountAppDataDirs = JNI_FALSE;
    jboolean bindMountAppStorageDirs = JNI_FALSE;

    nativeForkAndSpecialize_pre(env, clazz, uid, gid, gids, runtime_flags, rlimits, mount_external,
                                se_info, se_name, fdsToClose, fdsToIgnore, is_child_zygote,
                                instructionSet, appDataDir, isTopApp, pkgDataInfoList,
                                whitelistedDataInfoList,
                                bindMountAppDataDirs, bindMountAppStorageDirs);

    jint res = ((nativeForkAndSpecialize_samsung_p_t *) jni::zygote::nativeForkAndSpecialize->fnPtr)(
            env, clazz, uid, gid, gids, runtime_flags, rlimits, mount_external, se_info, category,
            accessInfo, se_name, fdsToClose, fdsToIgnore, is_child_zygote, instructionSet,
            appDataDir);

    nativeForkAndSpecialize_post(env, clazz, uid, is_child_zygote, res);
    return res;
}

[[clang::no_stack_protector]]
jint nativeForkAndSpecialize_samsung_o(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jint category, jint accessInfo,
        jstring se_name, jintArray fdsToClose, jintArray fdsToIgnore, jstring instructionSet,
        jstring appDataDir) {

    jboolean is_child_zygote = JNI_FALSE;
    jboolean isTopApp = JNI_FALSE;
    jobjectArray pkgDataInfoList = nullptr;
    jobjectArray whitelistedDataInfoList = nullptr;
    jboolean bindMountAppDataDirs = JNI_FALSE;
    jboolean bindMountAppStorageDirs = JNI_FALSE;

    nativeForkAndSpecialize_pre(env, clazz, uid, gid, gids, debug_flags, rlimits, mount_external,
                                se_info, se_name, fdsToClose, fdsToIgnore, is_child_zygote,
                                instructionSet, appDataDir, isTopApp, pkgDataInfoList,
                                whitelistedDataInfoList,
                                bindMountAppDataDirs, bindMountAppStorageDirs);

    jint res = ((nativeForkAndSpecialize_samsung_o_t *) jni::zygote::nativeForkAndSpecialize->fnPtr)(
            env, clazz, uid, gid, gids, debug_flags, rlimits, mount_external, se_info, category,
            accessInfo, se_name, fdsToClose, fdsToIgnore, instructionSet, appDataDir);

    nativeForkAndSpecialize_post(env, clazz, uid, is_child_zygote, res);
    return res;
}

[[clang::no_stack_protector]]
jint nativeForkAndSpecialize_samsung_n(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jint category, jint accessInfo,
        jstring se_name, jintArray fdsToClose, jstring instructionSet, jstring appDataDir,
        jint a1) {

    jintArray fdsToIgnore = nullptr;
    jboolean is_child_zygote = JNI_FALSE;
    jboolean isTopApp = JNI_FALSE;
    jobjectArray pkgDataInfoList = nullptr;
    jobjectArray whitelistedDataInfoList = nullptr;
    jboolean bindMountAppDataDirs = JNI_FALSE;
    jboolean bindMountAppStorageDirs = JNI_FALSE;

    nativeForkAndSpecialize_pre(env, clazz, uid, gid, gids, debug_flags, rlimits, mount_external,
                                se_info, se_name, fdsToClose, fdsToIgnore, is_child_zygote,
                                instructionSet, appDataDir, isTopApp, pkgDataInfoList,
                                whitelistedDataInfoList,
                                bindMountAppDataDirs, bindMountAppStorageDirs);

    jint res = ((nativeForkAndSpecialize_samsung_n_t *) jni::zygote::nativeForkAndSpecialize->fnPtr)(
            env, clazz, uid, gid, gids, debug_flags, rlimits, mount_external, se_info, category,
            accessInfo, se_name, fdsToClose, instructionSet, appDataDir, a1);

    nativeForkAndSpecialize_post(env, clazz, uid, is_child_zygote, res);
    return res;
}

[[clang::no_stack_protector]]
jint nativeForkAndSpecialize_samsung_m(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint debug_flags,
        jobjectArray rlimits, jint mount_external, jstring se_info, jint category, jint accessInfo,
        jstring se_name, jintArray fdsToClose, jstring instructionSet, jstring appDataDir) {

    jintArray fdsToIgnore = nullptr;
    jboolean is_child_zygote = JNI_FALSE;
    jboolean isTopApp = JNI_FALSE;
    jobjectArray pkgDataInfoList = nullptr;
    jobjectArray whitelistedDataInfoList = nullptr;
    jboolean bindMountAppDataDirs = JNI_FALSE;
    jboolean bindMountAppStorageDirs = JNI_FALSE;

    nativeForkAndSpecialize_pre(env, clazz, uid, gid, gids, debug_flags, rlimits, mount_external,
                                se_info, se_name, fdsToClose, fdsToIgnore, is_child_zygote,
                                instructionSet, appDataDir, isTopApp, pkgDataInfoList,
                                whitelistedDataInfoList,
                                bindMountAppDataDirs, bindMountAppStorageDirs);

    jint res = ((nativeForkAndSpecialize_samsung_m_t *) jni::zygote::nativeForkAndSpecialize->fnPtr)(
            env, clazz, uid, gid, gids, debug_flags, rlimits, mount_external, se_info, category,
            accessInfo, se_name, fdsToClose, instructionSet, appDataDir);

    nativeForkAndSpecialize_post(env, clazz, uid, is_child_zygote, res);
    return res;
}

// -----------------------------------------------------------------
[[clang::no_stack_protector]]
void nativeSpecializeAppProcess_q(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtimeFlags,
        jobjectArray rlimits, jint mountExternal, jstring seInfo, jstring niceName,
        jboolean startChildZygote, jstring instructionSet, jstring appDataDir) {

    jboolean isTopApp = JNI_FALSE;
    jobjectArray pkgDataInfoList = nullptr;
    jobjectArray whitelistedDataInfoList = nullptr;
    jboolean bindMountAppDataDirs = JNI_FALSE;
    jboolean bindMountAppStorageDirs = JNI_FALSE;

    nativeSpecializeAppProcess_pre(
            env, clazz, uid, gid, gids, runtimeFlags, rlimits, mountExternal, seInfo, niceName,
            startChildZygote, instructionSet, appDataDir, isTopApp, pkgDataInfoList,
            whitelistedDataInfoList, bindMountAppDataDirs, bindMountAppStorageDirs);

    ((nativeSpecializeAppProcess_q_t *) jni::zygote::nativeSpecializeAppProcess->fnPtr)(
            env, clazz, uid, gid, gids, runtimeFlags, rlimits, mountExternal, seInfo, niceName,
            startChildZygote, instructionSet, appDataDir);

    nativeSpecializeAppProcess_post(env, clazz, uid, startChildZygote);
}

[[clang::no_stack_protector]]
void nativeSpecializeAppProcess_q_alternative(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtimeFlags,
        jobjectArray rlimits, jint mountExternal, jstring seInfo, jstring niceName,
        jboolean startChildZygote, jstring instructionSet, jstring appDataDir,
        jboolean isTopApp) {

    jobjectArray pkgDataInfoList = nullptr;
    jobjectArray whitelistedDataInfoList = nullptr;
    jboolean bindMountAppDataDirs = JNI_FALSE;
    jboolean bindMountAppStorageDirs = JNI_FALSE;

    nativeSpecializeAppProcess_pre(
            env, clazz, uid, gid, gids, runtimeFlags, rlimits, mountExternal, seInfo, niceName,
            startChildZygote, instructionSet, appDataDir, isTopApp, pkgDataInfoList,
            whitelistedDataInfoList, bindMountAppDataDirs, bindMountAppStorageDirs);

    ((nativeSpecializeAppProcess_q_alternative_t *) jni::zygote::nativeSpecializeAppProcess->fnPtr)(
            env, clazz, uid, gid, gids, runtimeFlags, rlimits, mountExternal, seInfo, niceName,
            startChildZygote, instructionSet, appDataDir, isTopApp);

    nativeSpecializeAppProcess_post(env, clazz, uid, startChildZygote);
}

[[clang::no_stack_protector]]
void nativeSpecializeAppProcess_r(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtimeFlags,
        jobjectArray rlimits, jint mountExternal, jstring seInfo, jstring niceName,
        jboolean startChildZygote, jstring instructionSet, jstring appDataDir,
        jboolean isTopApp, jobjectArray pkgDataInfoList, jobjectArray whitelistedDataInfoList,
        jboolean bindMountAppDataDirs, jboolean bindMountAppStorageDirs) {

    nativeSpecializeAppProcess_pre(
            env, clazz, uid, gid, gids, runtimeFlags, rlimits, mountExternal, seInfo, niceName,
            startChildZygote, instructionSet, appDataDir, isTopApp, pkgDataInfoList,
            whitelistedDataInfoList, bindMountAppDataDirs, bindMountAppStorageDirs);

    ((nativeSpecializeAppProcess_r_t *) jni::zygote::nativeSpecializeAppProcess->fnPtr)(
            env, clazz, uid, gid, gids, runtimeFlags, rlimits, mountExternal, seInfo, niceName,
            startChildZygote, instructionSet, appDataDir, isTopApp, pkgDataInfoList,
            whitelistedDataInfoList, bindMountAppDataDirs, bindMountAppStorageDirs);

    nativeSpecializeAppProcess_post(env, clazz, uid, startChildZygote);
}

[[clang::no_stack_protector]]
void nativeSpecializeAppProcess_r_dp3(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtimeFlags,
        jobjectArray rlimits, jint mountExternal, jstring seInfo, jstring niceName,
        jboolean startChildZygote, jstring instructionSet, jstring appDataDir,
        jboolean isTopApp, jobjectArray pkgDataInfoList, jboolean bindMountAppStorageDirs) {

    jobjectArray whitelistedDataInfoList = nullptr;
    jboolean bindMountAppDataDirs = JNI_FALSE;

    nativeSpecializeAppProcess_pre(
            env, clazz, uid, gid, gids, runtimeFlags, rlimits, mountExternal, seInfo, niceName,
            startChildZygote, instructionSet, appDataDir, isTopApp, pkgDataInfoList,
            whitelistedDataInfoList, bindMountAppDataDirs, bindMountAppStorageDirs);

    ((nativeSpecializeAppProcess_r_dp3_t *) jni::zygote::nativeSpecializeAppProcess->fnPtr)(
            env, clazz, uid, gid, gids, runtimeFlags, rlimits, mountExternal, seInfo, niceName,
            startChildZygote, instructionSet, appDataDir, isTopApp, pkgDataInfoList,
            bindMountAppStorageDirs);

    nativeSpecializeAppProcess_post(env, clazz, uid, startChildZygote);
}

[[clang::no_stack_protector]]
void nativeSpecializeAppProcess_r_dp2(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtimeFlags,
        jobjectArray rlimits, jint mountExternal, jstring seInfo, jstring niceName,
        jboolean startChildZygote, jstring instructionSet, jstring appDataDir,
        jboolean isTopApp, jobjectArray pkgDataInfoList) {

    jobjectArray whitelistedDataInfoList = nullptr;
    jboolean bindMountAppDataDirs = JNI_FALSE;
    jboolean bindMountAppStorageDirs = JNI_FALSE;

    nativeSpecializeAppProcess_pre(
            env, clazz, uid, gid, gids, runtimeFlags, rlimits, mountExternal, seInfo, niceName,
            startChildZygote, instructionSet, appDataDir, isTopApp, pkgDataInfoList,
            whitelistedDataInfoList, bindMountAppDataDirs, bindMountAppStorageDirs);

    ((nativeSpecializeAppProcess_r_dp2_t *) jni::zygote::nativeSpecializeAppProcess->fnPtr)(
            env, clazz, uid, gid, gids, runtimeFlags, rlimits, mountExternal, seInfo, niceName,
            startChildZygote, instructionSet, appDataDir, isTopApp, pkgDataInfoList);

    nativeSpecializeAppProcess_post(env, clazz, uid, startChildZygote);
}

[[clang::no_stack_protector]]
void nativeSpecializeAppProcess_samsung_q(
        JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids, jint runtimeFlags,
        jobjectArray rlimits, jint mountExternal, jstring seInfo, jint space, jint accessInfo,
        jstring niceName, jboolean startChildZygote, jstring instructionSet, jstring appDataDir) {

    jboolean isTopApp = JNI_FALSE;
    jobjectArray pkgDataInfoList = nullptr;
    jobjectArray whitelistedDataInfoList = nullptr;
    jboolean bindMountAppDataDirs = JNI_FALSE;
    jboolean bindMountAppStorageDirs = JNI_FALSE;

    nativeSpecializeAppProcess_pre(
            env, clazz, uid, gid, gids, runtimeFlags, rlimits, mountExternal, seInfo, niceName,
            startChildZygote, instructionSet, appDataDir, isTopApp, pkgDataInfoList,
            whitelistedDataInfoList, bindMountAppDataDirs, bindMountAppStorageDirs);

    ((nativeSpecializeAppProcess_samsung_t *) jni::zygote::nativeSpecializeAppProcess->fnPtr)(
            env, clazz, uid, gid, gids, runtimeFlags, rlimits, mountExternal, seInfo, space,
            accessInfo, niceName, startChildZygote, instructionSet, appDataDir);

    nativeSpecializeAppProcess_post(env, clazz, uid, startChildZygote);
}

// -----------------------------------------------------------------
[[clang::no_stack_protector]]
jint nativeForkSystemServer(
        JNIEnv *env, jclass clazz, uid_t uid, gid_t gid, jintArray gids, jint runtimeFlags,
        jobjectArray rlimits, jlong permittedCapabilities, jlong effectiveCapabilities) {

    nativeForkSystemServer_pre(
            env, clazz, uid, gid, gids, runtimeFlags, rlimits, permittedCapabilities,
            effectiveCapabilities);

    jint res = ((nativeForkSystemServer_t *) jni::zygote::nativeForkSystemServer->fnPtr)(
            env, clazz, uid, gid, gids, runtimeFlags, rlimits, permittedCapabilities,
            effectiveCapabilities);

    nativeForkSystemServer_post(env, clazz, res);
    return res;
}

[[clang::no_stack_protector]]
jint nativeForkSystemServer_samsung_q(
        JNIEnv *env, jclass cls, uid_t uid, gid_t gid, jintArray gids, jint runtimeFlags,
        jint space, jint accessInfo, jobjectArray rlimits, jlong permittedCapabilities,
        jlong effectiveCapabilities) {

    nativeForkSystemServer_pre(
            env, cls, uid, gid, gids, runtimeFlags, rlimits, permittedCapabilities,
            effectiveCapabilities);

    jint res = ((nativeForkSystemServer_samsung_q_t *) jni::zygote::nativeForkSystemServer->fnPtr)(
            env, cls, uid, gid, gids, runtimeFlags, space, accessInfo, rlimits,
            permittedCapabilities,
            effectiveCapabilities);

    nativeForkSystemServer_post(env, cls, res);
    return res;
}
