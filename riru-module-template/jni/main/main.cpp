#include <jni.h>
#include <sys/types.h>

extern "C" {
__attribute__((visibility("default")))
void nativeForkAndSpecializePre(JNIEnv *env, jclass clazz, jint uid, jint gid, jintArray gids,
                                jint runtime_flags, jobjectArray rlimits, jint mount_external,
                                jstring se_info, jstring se_name, jintArray fdsToClose,
                                jintArray fdsToIgnore,
                                jboolean is_child_zygote, jstring instructionSet,
                                jstring appDataDir) {

}

__attribute__((visibility("default")))
int nativeForkAndSpecializePost(JNIEnv *env, jclass clazz, jint res) {
    if (res ==  0) {
        // in app process
    } else {
        // in zygote process, res is child pid
        // don't print log here, see https://github.com/RikkaApps/Riru/blob/77adfd6a4a6a81bfd20569c910bc4854f2f84f5e/riru-core/jni/main/jni_native_method.cpp#L55-L66
    }
    return 0;
}

__attribute__((visibility("default")))
void nativeForkSystemServerPre(JNIEnv *env, jclass clazz, uid_t uid, gid_t gid, jintArray gids,
                         jint debug_flags, jobjectArray rlimits, jlong permittedCapabilities,
                         jlong effectiveCapabilities) {
}

__attribute__((visibility("default")))
int nativeForkSystemServerPost(JNIEnv *env, jclass clazz, jint res) {
    if (res ==  0) {
        // in system server process
    } else {
        // in zygote process, res is child pid
        // don't print log here, see https://github.com/RikkaApps/Riru/blob/77adfd6a4a6a81bfd20569c910bc4854f2f84f5e/riru-core/jni/main/jni_native_method.cpp#L55-L66
    }
    return 0;
}
}
