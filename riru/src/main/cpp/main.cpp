#include <xhook.h>
#include <sys/system_properties.h>
#include <dlfcn.h>
#include <plt.h>
#include <android_prop.h>
#include "misc.h"
#include "jni_native_method.h"
#include "logging.h"
#include "wrap.h"
#include "module.h"
#include "api.h"
#include "native_method.h"
#include "hide_utils.h"
#include "status.h"
#include "config.h"
#include "magisk.h"

static bool useTableOverride = false;

using GetJniNativeInterface_t = const JNINativeInterface *();
using SetTableOverride_t = void(JNINativeInterface *);
using RegisterNatives_t = jint(JNIEnv *, jclass, const JNINativeMethod *, jint);

static SetTableOverride_t *setTableOverride = nullptr;
static RegisterNatives_t *old_RegisterNatives = nullptr;

static JNINativeMethod *onRegisterZygote(const char *className, const JNINativeMethod *methods, int numMethods) {

    auto *newMethods = new JNINativeMethod[numMethods];
    memcpy(newMethods, methods, sizeof(JNINativeMethod) * numMethods);

    JNINativeMethod method;
    for (int i = 0; i < numMethods; ++i) {
        method = methods[i];

        if (strcmp(method.name, "nativeForkAndSpecialize") == 0) {
            JNI::Zygote::nativeForkAndSpecialize = new JNINativeMethod{method.name, method.signature, method.fnPtr};

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
                api::setNativeMethodFunc(
                        get_modules()->at(0)->token, className, newMethods[i].name, newMethods[i].signature, newMethods[i].fnPtr);
            }
            Status::WriteMethod(Status::Method::forkAndSpecialize, replaced, method.signature);
        } else if (strcmp(method.name, "nativeSpecializeAppProcess") == 0) {
            JNI::Zygote::nativeSpecializeAppProcess = new JNINativeMethod{method.name, method.signature, method.fnPtr};

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
                api::setNativeMethodFunc(
                        get_modules()->at(0)->token, className, newMethods[i].name, newMethods[i].signature, newMethods[i].fnPtr);
            }
            Status::WriteMethod(Status::Method::specializeAppProcess, replaced, method.signature);
        } else if (strcmp(method.name, "nativeForkSystemServer") == 0) {
            JNI::Zygote::nativeForkSystemServer = new JNINativeMethod{method.name, method.signature, method.fnPtr};

            if (strcmp(nativeForkSystemServer_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkSystemServer;
            else if (strcmp(nativeForkSystemServer_samsung_q_sig, method.signature) == 0)
                newMethods[i].fnPtr = (void *) nativeForkSystemServer_samsung_q;
            else
                LOGW("found nativeForkSystemServer but signature %s mismatch", method.signature);

            auto replaced = newMethods[i].fnPtr != methods[i].fnPtr;
            if (replaced) {
                LOGI("replaced com.android.internal.os.Zygote#nativeForkSystemServer");
                api::setNativeMethodFunc(
                        get_modules()->at(0)->token, className, newMethods[i].name, newMethods[i].signature, newMethods[i].fnPtr);
            }
            Status::WriteMethod(Status::Method::forkSystemServer, replaced, method.signature);
        }
    }

    return newMethods;
}

static JNINativeMethod *onRegisterSystemProperties(const char *className, const JNINativeMethod *methods, int numMethods) {

    auto *newMethods = new JNINativeMethod[numMethods];
    memcpy(newMethods, methods, sizeof(JNINativeMethod) * numMethods);

    JNINativeMethod method;
    for (int i = 0; i < numMethods; ++i) {
        method = methods[i];

        if (strcmp(method.name, "native_set") == 0) {
            JNI::SystemProperties::set = new JNINativeMethod{method.name, method.signature, method.fnPtr};

            if (strcmp("(Ljava/lang/String;Ljava/lang/String;)V", method.signature) == 0)
                newMethods[i].fnPtr = (void *) SystemProperties_set;
            else
                LOGW("found native_set but signature %s mismatch", method.signature);

            if (newMethods[i].fnPtr != methods[i].fnPtr) {
                LOGI("replaced android.os.SystemProperties#native_set");

                api::setNativeMethodFunc(
                        get_modules()->at(0)->token, className, newMethods[i].name, newMethods[i].signature, newMethods[i].fnPtr);
            }
        }
    }
    return newMethods;
}

static JNINativeMethod *handleRegisterNative(const char *className, const JNINativeMethod *methods, int numMethods) {
    if (strcmp("com/android/internal/os/Zygote", className) == 0) {
        return onRegisterZygote(className, methods, numMethods);
    } else if (strcmp("android/os/SystemProperties", className) == 0) {
        // hook android.os.SystemProperties#native_set to prevent a critical problem on Android 9
        // see comment of SystemProperties_set in jni_native_method.cpp for detail
        return onRegisterSystemProperties(className, methods, numMethods);
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
    api::putNativeMethod(className, methods, numMethods);

    LOGD("jniRegisterNativeMethods %s", className);

    JNINativeMethod *newMethods = handleRegisterNative(className, methods, numMethods);
    int res = old_jniRegisterNativeMethods(env, className, newMethods ? newMethods : methods, numMethods);
    /*if (!newMethods) {
        NativeMethod::jniRegisterNativeMethodsPost(env, className, methods, numMethods);
    }*/
    delete newMethods;
    return res;
}

static jclass zygoteClass;
static jclass systemPropertiesClass;

static void prepareClassesForRegisterNativeHook(JNIEnv *env) {
    static bool called = false;
    if (called) return;
    called = true;

    auto _zygoteClass = env->FindClass("com/android/internal/os/Zygote");
    auto _systemPropertiesClass = env->FindClass("android/os/SystemProperties");

    // There are checks that enforces no local refs exists during Runtime::Start, make them global ref
    zygoteClass = (jclass) env->NewGlobalRef(_zygoteClass);
    systemPropertiesClass = (jclass) env->NewGlobalRef(_systemPropertiesClass);

    env->DeleteLocalRef(_zygoteClass);
    env->DeleteLocalRef(_systemPropertiesClass);
}

static int new_RegisterNative(JNIEnv *env, jclass cls, const JNINativeMethod *methods, jint numMethods) {
    prepareClassesForRegisterNativeHook(env);

    const char *className;
    if (zygoteClass != nullptr && env->IsSameObject(zygoteClass, cls)) {
        className = "com/android/internal/os/Zygote";
        LOGD("RegisterNative %s", className);
        env->DeleteGlobalRef(zygoteClass);
        zygoteClass = nullptr;
    } else if (systemPropertiesClass != nullptr && env->IsSameObject(systemPropertiesClass, cls)) {
        className = "android/os/SystemProperties";
        LOGD("RegisterNative %s", className);
        env->DeleteGlobalRef(systemPropertiesClass);
        systemPropertiesClass = nullptr;
    } else {
        className = "";
    }

    JNINativeMethod *newMethods = handleRegisterNative(className, methods, numMethods);
    auto res = old_RegisterNatives(env, cls, newMethods ? newMethods : methods, numMethods);
    delete newMethods;
    return res;
}

#define restoreMethod(_cls, method) \
    if (JNI::_cls::method != nullptr) { \
        if (old_jniRegisterNativeMethods) \
        old_jniRegisterNativeMethods(env, JNI::_cls::classname, JNI::_cls::method, 1); \
        delete JNI::_cls::method; \
    }

void restore_replaced_func(JNIEnv *env) {
    if (useTableOverride) {
        setTableOverride(nullptr);
    } else {
        xhook_register(".*\\libandroid_runtime.so$", "jniRegisterNativeMethods",
                       (void *) old_jniRegisterNativeMethods,
                       nullptr);
        if (xhook_refresh(0) == 0) {
            xhook_clear();
            LOGD("hook removed");
        }
    }

    restoreMethod(Zygote, nativeForkAndSpecialize)
    restoreMethod(Zygote, nativeSpecializeAppProcess)
    restoreMethod(Zygote, nativeForkSystemServer)
    restoreMethod(SystemProperties, set)
}

extern "C" void constructor() __attribute__((constructor));

void constructor() {
#ifdef DEBUG_APP
    hide::hide_modules(nullptr, 0);
#endif

    if (getuid() != 0)
        return;

    char cmdline[ARG_MAX + 1];
    get_self_cmdline(cmdline, 0);

    if (strcmp(cmdline, "zygote") != 0
        && strcmp(cmdline, "zygote32") != 0
        && strcmp(cmdline, "zygote64") != 0
        && strcmp(cmdline, "usap32") != 0
        && strcmp(cmdline, "usap64") != 0) {
        LOGW("not zygote (cmdline=%s)", cmdline);
        return;
    }

    LOGI("Riru %s (%d) in %s", RIRU_VERSION_NAME, RIRU_VERSION_CODE, cmdline);
    LOGI("Magisk tmpfs path is %s", Magisk::GetPath());
    LOGI("Android %s (api %d, preview_api %d)", AndroidProp::GetRelease(), AndroidProp::GetApiLevel(), AndroidProp::GetPreviewApiLevel());

    XHOOK_REGISTER(".*\\libandroid_runtime.so$", jniRegisterNativeMethods);

    if (xhook_refresh(0) == 0) {
        xhook_clear();
        LOGI("hook installed");
    } else {
        LOGE("failed to refresh hook");
    }

    useTableOverride = old_jniRegisterNativeMethods == nullptr;

    if (useTableOverride) {
        LOGI("no jniRegisterNativeMethods");

        auto *GetJniNativeInterface = (GetJniNativeInterface_t *) plt_dlsym("_ZN3art21GetJniNativeInterfaceEv", nullptr);
        setTableOverride = (SetTableOverride_t *) plt_dlsym("_ZN3art9JNIEnvExt16SetTableOverrideEPK18JNINativeInterface", nullptr);

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

        auto handle = dlopen("libnativehelper.so", 0);
        if (handle) {
            old_jniRegisterNativeMethods = (jniRegisterNativeMethods_t *) dlsym(handle, "jniRegisterNativeMethods");
        }
    }

    Modules::Load();

    Status::WriteSelfAndModules();
}
