#pragma once

#include <jni.h>
#include "riru.h"

void put_native_method(const char *className, const JNINativeMethod *methods, int numMethods);

#define KEEP __attribute__((visibility("hidden"))) __attribute__((used))

void *riru_get_func(uint32_t token, const char *name) KEEP;

void *riru_get_native_method_func(
        uint32_t token, const char *className, const char *name, const char *signature) KEEP;

void riru_set_func(uint32_t token, const char *name, void *func) KEEP;

void riru_set_native_method_func(
        uint32_t token, const char *className, const char *name, const char *signature, void *func) KEEP;

const JNINativeMethod *riru_get_original_native_methods(
        const char *className, const char *name, const char *signature) KEEP;