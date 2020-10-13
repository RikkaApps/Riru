#pragma once

#include <jni.h>
#include <riru.h>

#define KEEP __attribute__((visibility("hidden"))) __attribute__((used))

namespace api {

    void putNativeMethod(const char *className, const JNINativeMethod *methods, int numMethods);

    void *getFunc(uint32_t token, const char *name) KEEP;

    void *getNativeMethodFunc(
            uint32_t token, const char *className, const char *name, const char *signature) KEEP;

    void setFunc(uint32_t token, const char *name, void *func) KEEP;

    void setNativeMethodFunc(
            uint32_t token, const char *className, const char *name, const char *signature, void *func) KEEP;

    const JNINativeMethod *getOriginalNativeMethod(
            const char *className, const char *name, const char *signature) KEEP;

    void putGlobalValue(const char *key, void *value);

    void *getGlobalValue(const char *key);
}