#ifndef _JNI_NATIVE_METHOD_H
#define _JNI_NATIVE_METHOD_H

#include <jni.h>

extern void* _nativeForkAndSpecialize;
extern void* _nativeForkSystemServer;

const static char* nativeForkAndSpecialize_marshmallow_sig = "(II[II[[IILjava/lang/String;Ljava/lang/String;[ILjava/lang/String;Ljava/lang/String;)I";

typedef jint (*nativeForkAndSpecialize_marshmallow_t)(JNIEnv*, jclass, jint, jint, jintArray, jint, jobjectArray,
                                                      jint, jstring, jstring, jintArray, jstring, jstring);

const static char* nativeForkAndSpecialize_oreo_sig = "(II[II[[IILjava/lang/String;Ljava/lang/String;[I[ILjava/lang/String;Ljava/lang/String;)I";

typedef jint (*nativeForkAndSpecialize_oreo_t)(JNIEnv*, jclass, jint, jint, jintArray, jint, jobjectArray,
                                               jint, jstring, jstring, jintArray, jintArray, jstring, jstring);

const static char* nativeForkAndSpecialize_p_sig = "(II[II[[IILjava/lang/String;Ljava/lang/String;[I[IZLjava/lang/String;Ljava/lang/String;)I";

typedef jint (*nativeForkAndSpecialize_p_t)(JNIEnv*, jclass, jint, jint, jintArray, jint, jobjectArray,
                                            jint, jstring, jstring, jintArray, jintArray, jboolean,
                                            jstring, jstring);

extern jint nativeForkAndSpecialize_marshmallow(JNIEnv *env, jclass clazz, jint uid, jint gid,
                                                jintArray gids,
                                                jint debug_flags, jobjectArray rlimits,
                                                jint mount_external, jstring se_info,
                                                jstring se_name,
                                                jintArray fdsToClose, jstring instructionSet,
                                                jstring appDataDir);

extern jint nativeForkAndSpecialize_oreo(JNIEnv *env, jclass clazz, jint uid, jint gid,
                                         jintArray gids,
                                         jint debug_flags, jobjectArray rlimits,
                                         jint mount_external, jstring se_info, jstring se_name,
                                         jintArray fdsToClose,
                                         jintArray fdsToIgnore,
                                         jstring instructionSet, jstring appDataDir);

extern jint nativeForkAndSpecialize_p(JNIEnv *env, jclass clazz, jint uid, jint gid,
                                      jintArray gids,
                                      jint runtime_flags, jobjectArray rlimits,
                                      jint mount_external, jstring se_info, jstring se_name,
                                      jintArray fdsToClose, jintArray fdsToIgnore,
                                      jboolean is_child_zygote,
                                      jstring instructionSet, jstring appDataDir);

const static char* nativeForkSystemServer_sig = "(II[II[[IJJ)I";

typedef jint (*nativeForkSystemServer_t)(JNIEnv*, jclass, uid_t, gid_t, jintArray,
                                         jint, jobjectArray, jlong, jlong);

extern jint nativeForkSystemServer(JNIEnv* env, jclass, uid_t uid, gid_t gid, jintArray gids,
                                   jint debug_flags, jobjectArray rlimits, jlong permittedCapabilities,
                                   jlong effectiveCapabilities);

#endif // _JNI_NATIVE_METHOD_H
