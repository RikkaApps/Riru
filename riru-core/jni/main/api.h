#ifndef RIRU_API_H
#define RIRU_API_H

#include <vector>

#include "module.h"

__attribute__((visibility("hidden")))
std::vector<module *> *get_modules();

__attribute__((visibility("hidden")))
void put_native_method(const char *className, const JNINativeMethod *methods, int numMethods);

#ifdef __cplusplus
extern "C" {
#endif
__attribute__((visibility("default")))
void *riru_get_func(const char *module_name, const char *name);

__attribute__((visibility("default")))
void *riru_get_native_method_func(const char *module_name, const char *className, const char *name,
                                  const char *signature);

__attribute__((visibility("default")))
void riru_set_func(const char *module_name, const char *name, void* func);

__attribute__((visibility("default")))
void riru_set_native_method_func(const char *module_name, const char *className, const char *name,
                                 const char *signature, void* func);

__attribute__((visibility("default")))
const JNINativeMethod *riru_get_original_native_methods(const char *className, const char *name,
                                                        const char *signature);

__attribute__((visibility("default")))
const JNINativeMethod *riru_get_original_native_method_by_name(const char *className,
                                                               const char *name);

__attribute__((visibility("default")))
const JNINativeMethod *riru_get_original_native_methods_by_class(const char *className);

__attribute__((visibility("default")))
int riru_is_zygote_methods_replaced();

__attribute__((visibility("default")))
int riru_get_nativeForkAndSpecialize_calls_count();

__attribute__((visibility("default")))
int riru_get_nativeForkSystemServer_calls_count();

#ifdef __cplusplus
}
#endif

#endif // RIRU_API_H
