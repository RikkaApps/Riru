#include <cstring>
#include <string>
#include <map>
#include <vector>
#include <jni.h>

#include "logging.h"
#include "module.h"
#include "api.h"

struct JNINativeMethodHolder {
    const JNINativeMethod *methods;
    int count;

    JNINativeMethodHolder(const JNINativeMethod *methods, int count) :
            methods(methods), count(count) {}
};

static auto *native_methods = new std::map<std::string, JNINativeMethodHolder*>();

std::vector<module *> *get_modules() {
    static auto *modules = new std::vector<module *>({new module(strdup(MODULE_NAME_CORE))});
    return modules;
}

void put_native_method(const char *className, const JNINativeMethod *methods, int numMethods) {
    (*native_methods)[className] = new JNINativeMethodHolder(methods, numMethods);
}

static unsigned long get_module_index(const char *name) {
    if (!name)
        return 0;

    for (unsigned long i = 0; i < get_modules()->size(); ++i) {
        if (strcmp(get_modules()->at(i)->name, name) == 0)
            return i + 1;
    }
    return 0;
}

extern "C" {
const JNINativeMethod *riru_get_original_native_methods(const char *className, const char *name,
                                                        const char *signature) {
    auto it = native_methods->find(className);
    if (it != native_methods->end()) {
        if (!name && !signature)
            return it->second->methods;

        auto holder = it->second;
        for (int i = 0; i < holder->count; ++i) {
            auto method = &holder->methods[i];
            if ((!name || strcmp(method->name, name) == 0)
                && (!signature || strcmp(method->signature, signature) == 0))
                return method;
        }
    }
    return nullptr;
}

void *riru_get_func(const char *module_name, const char *name) {
    unsigned long index = get_module_index(module_name);
    if (index == 0)
        return nullptr;

    index -= 1;

    //LOGV("get_func %s %s", module_name, name);

    // find if it is set by previous modules
    if (index != 0) {
        for (unsigned long i = index - 1; i >= 0; --i) {
            auto module = get_modules()->at(i);
            auto it = module->funcs->find(name);
            if (module->funcs->end() != it)
                return it->second;

            if (i == 0) break;
        }
    }

    return nullptr;
}

void *riru_get_native_method_func(const char *module_name, const char *className, const char *name,
                                  const char *signature) {
    unsigned long index = get_module_index(module_name);
    if (index == 0)
        return nullptr;

    index -= 1;

    //LOGV("get_func %s %s %s %s", module_name, className, name, signature);

    // find if it is set by previous modules
    if (index != 0) {
        for (unsigned long i = index - 1; i >= 0; --i) {
            auto module = get_modules()->at(i);
            auto it = module->funcs->find(std::string(className) + name + signature);
            if (module->funcs->end() != it)
                return it->second;

            if (i == 0) break;
        }
    }

    const JNINativeMethod *jniNativeMethod = riru_get_original_native_methods(className, name,
                                                                              signature);
    return jniNativeMethod ? jniNativeMethod->fnPtr : nullptr;
}

void riru_set_func(const char *module_name, const char *name, void *func) {
    unsigned long index = get_module_index(module_name);
    if (index == 0)
        return;

    //LOGV("set_func %s %s %p", module_name, name, func);

    auto module = get_modules()->at(index - 1);
    (*module->funcs)[name] = func;
}

void riru_set_native_method_func(const char *module_name, const char *className, const char *name,
                                 const char *signature, void *func) {
    riru_set_func(module_name, (std::string(className) + name + signature).c_str(), func);
}
}
