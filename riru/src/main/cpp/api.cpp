#include <cstring>
#include <map>
#include <vector>
#include <jni.h>

#include "logging.h"
#include "module.h"
#include "api.h"

namespace LegacyApiStubs {

    const JNINativeMethod *getOriginalNativeMethod(
            const char *className, const char *name, const char *signature) {
        return nullptr;
    }

    void *getFunc(uint32_t token, const char *name) {
        return nullptr;
    }

    void *getNativeMethodFunc(
            uint32_t token, const char *className, const char *name, const char *signature) {
        return nullptr;
    }

    void setFunc(uint32_t token, const char *name, void *func) {
    }

    void setNativeMethodFunc(
            uint32_t token, const char *className, const char *name, const char *signature, void *func) {
    }

    void putGlobalValue(const char *key, void *value) {
    }

    void *getGlobalValue(const char *key) {
        return nullptr;
    }
}