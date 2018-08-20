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
    int unload = 1;
    return unload;
}

__attribute__((visibility("default")))
void forkSystemServerPre(JNIEnv *env, jclass clazz, uid_t uid, gid_t gid, jintArray gids,
                         jint debug_flags, jobjectArray rlimits, jlong permittedCapabilities,
                         jlong effectiveCapabilities) {
}

__attribute__((visibility("default")))
int forkSystemServerPost(JNIEnv *env, jclass clazz, jint res) {
    int unload = 1;
    return unload;
}
}
