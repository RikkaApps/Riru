#ifndef RIRU_H
#define RIRU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef void(onModuleLoaded_t)();
typedef int(shouldSkipUid_t)(int uid);

struct RiruModule {

    int apiVersion = 0;
    int supportHide = 0;
    onModuleLoaded_t *onModuleLoaded = nullptr;
    shouldSkipUid_t *shouldSkipUid = nullptr;
    void *forkAndSpecializePre = nullptr;
    void *forkAndSpecializePost = nullptr;
    void *forkSystemServerPre = nullptr;
    void *forkSystemServerPost = nullptr;
    void *specializeAppProcessPre = nullptr;
    void *specializeAppProcessPost = nullptr;
};

typedef void *(RiruGetFunc_t)(uint32_t token, const char *name);
typedef void (RiruSetFunc_t)(uint32_t token, const char *name, void *func);
typedef void *(RiruGetJNINativeMethodFunc_t)(uint32_t token, const char *className, const char *name, const char *signature);
typedef void (RiruSetJNINativeMethodFunc_t)(uint32_t token, const char *className, const char *name, const char *signature, void *func);
typedef const JNINativeMethod *(RiruGetOriginalJNINativeMethodFunc_t)(const char *className, const char *name, const char *signature);

struct RiruFuncs {
    RiruGetFunc_t *getFunc;
    RiruGetJNINativeMethodFunc_t *getJNINativeMethodFunc;
    RiruSetFunc_t *setFunc;
    RiruSetJNINativeMethodFunc_t *setJNINativeMethodFunc;
    RiruGetOriginalJNINativeMethodFunc_t *getOriginalJNINativeMethodFunc;
};

struct RiruInit {

    int version;
    uint32_t token;
    RiruModule *module;
    RiruFuncs *funcs;
};

typedef void (RiruInit_t)(RiruInit *);

#ifdef __cplusplus
}
#endif

#endif //RIRU_H
