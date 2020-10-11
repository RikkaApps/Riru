#include <riru.h>

int riru_api_version;
RiruApiV9 *riru_api_v9;

void *riru_get_func(const char *name) {
    if (riru_api_version == 9) {
        return riru_api_v9->getFunc(riru_api_v9->token, name);
    }
    return NULL;
}

void *riru_get_native_method_func(const char *className, const char *name, const char *signature) {
    if (riru_api_version == 9) {
        return riru_api_v9->getJNINativeMethodFunc(riru_api_v9->token, className, name, signature);
    }
    return NULL;
}

const JNINativeMethod *riru_get_original_native_methods(const char *className, const char *name, const char *signature) {
    if (riru_api_version == 9) {
        return riru_api_v9->getOriginalJNINativeMethodFunc(className, name, signature);
    }
    return NULL;
}

void riru_set_func(const char *name, void *func) {
    if (riru_api_version == 9) {
        riru_api_v9->setFunc(riru_api_v9->token, name, func);
    }
}

void riru_set_native_method_func(const char *className, const char *name, const char *signature,
                                 void *func) {
    if (riru_api_version == 9) {
        riru_api_v9->setJNINativeMethodFunc(riru_api_v9->token, className, name, signature, func);
    }
}