#ifndef MODULE_H
#define MODULE_H

#include <map>

#define MODULE_NAME_CORE "core"

typedef void (*loaded_t)();

typedef void (*nativeForkAndSpecialize_pre_t)(JNIEnv *, jclass, jint, jint, jintArray, jint,
                                              jobjectArray,
                                              jint, jstring, jstring, jintArray, jintArray,
                                              jboolean,
                                              jstring, jstring);

typedef int (*nativeForkAndSpecialize_post_t)(JNIEnv *, jclass, jint);

typedef void (*nativeForkSystemServer_pre_t)(JNIEnv *, jclass, uid_t, gid_t, jintArray,
                                             jint, jobjectArray, jlong, jlong);

typedef int (*nativeForkSystemServer_post_t)(JNIEnv *, jclass, jint);

struct module {
    void *handle{};
    char *name;
    void *onModuleLoaded{};
    void *forkAndSpecializePre{};
    void *forkAndSpecializePost{};
    void *forkSystemServerPre{};
    void *forkSystemServerPost{};
    std::map<std::string, void *> * funcs;

    explicit module(char *name) : name(name) {
        funcs = new std::map<std::string, void *>();
    }
};

#endif // MODULE_H
