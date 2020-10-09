#pragma once

#include <jni.h>
#include <string>
#include <map>
#include <vector>
#include "api.h"

#define MODULE_NAME_CORE "core"

struct RiruModuleExt : RiruModule {

    void *handle{};
    const char *name;
    std::map<std::string, void *> *funcs;
    uint32_t token;

    explicit RiruModuleExt(const char *name) : name(name) {
        funcs = new std::map<std::string, void *>();
        token = (uintptr_t) name;
        apiVersion = 0;
        supportHide = 0;
        onModuleLoaded = nullptr;
        shouldSkipUid = nullptr;
        forkAndSpecializePre = nullptr;
        forkAndSpecializePost = nullptr;
        forkSystemServerPre = nullptr;
        forkSystemServerPost = nullptr;
        specializeAppProcessPre = nullptr;
        specializeAppProcessPost = nullptr;
    }
};


std::vector<RiruModuleExt *> *get_modules();

void load_modules();