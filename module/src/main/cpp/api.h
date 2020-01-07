#ifndef RIRU_API_H
#define RIRU_API_H

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EXPORT __attribute__((visibility("default"))) __attribute__((used))

int riru_get_version(void) EXPORT;

void *riru_get_func(const char *module_name, const char *name) EXPORT;

void *riru_get_native_method_func(const char *module_name, const char *className, const char *name,
                                  const char *signature) EXPORT;

void riru_set_func(const char *module_name, const char *name, void *func) EXPORT;

void riru_set_native_method_func(const char *module_name, const char *className, const char *name,
                                 const char *signature, void *func) EXPORT;

const JNINativeMethod *riru_get_original_native_methods(const char *className, const char *name,
                                                        const char *signature) EXPORT;

int riru_is_zygote_methods_replaced() EXPORT;

int riru_get_nativeForkAndSpecialize_calls_count() EXPORT;

int riru_get_nativeForkSystemServer_calls_count() EXPORT;

int riru_get_nativeSpecializeAppProcess_calls_count() EXPORT;

#ifdef __cplusplus
}
#endif

#endif // RIRU_API_H
