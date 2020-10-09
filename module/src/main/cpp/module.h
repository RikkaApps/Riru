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
        token = (uintptr_t) name + (uintptr_t) funcs;
    }
};


std::vector<RiruModuleExt *> *get_modules();

void load_modules();