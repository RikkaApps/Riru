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
#ifdef __cplusplus
}
#endif

#endif // RIRU_API_H
