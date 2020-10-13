#include <cstring>
#include <map>
#include <vector>
#include <jni.h>

#include "logging.h"
#include "module.h"
#include "api.h"

namespace api {

    struct JNINativeMethodHolder {
        const JNINativeMethod *methods;
        int count;

        JNINativeMethodHolder(const JNINativeMethod *methods, int count) :
                methods(methods), count(count) {}
    };

    static auto *native_methods = new std::map<std::string, JNINativeMethodHolder *>();

    void putNativeMethod(const char *className, const JNINativeMethod *methods, int numMethods) {
        (*native_methods)[className] = new JNINativeMethodHolder(methods, numMethods);
    }

    static unsigned long get_module_index(uint32_t token) {
        for (unsigned long i = 0; i < get_modules()->size(); ++i) {
            if (get_modules()->at(i)->token == token)
                return i + 1;
        }
        return 0;
    }

    const JNINativeMethod *getOriginalNativeMethod(
            const char *className, const char *name, const char *signature) {

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

    void *getFunc(uint32_t token, const char *name) {
        unsigned long index = get_module_index(token);
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

    void *getNativeMethodFunc(
            uint32_t token, const char *className, const char *name, const char *signature) {
        unsigned long index = get_module_index(token);
        if (index == 0)
            return nullptr;

        index -= 1;

        // find if it is set by previous modules
        char buf[4096];
        if (index != 0) {
            for (unsigned long i = index - 1; i >= 0; --i) {
                snprintf(buf, 4090, "%s%s%s", className, name, signature);
                auto module = get_modules()->at(i);
                auto it = module->funcs->find(std::string(className) + name + signature);
                if (module->funcs->end() != it)
                    return it->second;

                if (i == 0) break;
            }
        }

        const JNINativeMethod *jniNativeMethod = getOriginalNativeMethod(className, name,
                                                                         signature);
        return jniNativeMethod ? jniNativeMethod->fnPtr : nullptr;
    }

    void setFunc(uint32_t token, const char *name, void *func) {
        unsigned long index = get_module_index(token);
        if (index == 0)
            return;

        //LOGV("set_func %s %s %p", module_name, name, func);

        auto module = get_modules()->at(index - 1);
        (*module->funcs)[name] = func;
    }

    void setNativeMethodFunc(
            uint32_t token, const char *className, const char *name, const char *signature, void *func) {
        char buf[4096];
        snprintf(buf, 4090, "%s%s%s", className, name, signature);
        setFunc(token, buf, func);
    }

    static auto *global_values = new std::map<std::string, void *>();

    void putGlobalValue(const char *key, void *value) {
        if (value == nullptr) {
            global_values->erase(key);
        } else {
            (*global_values)[key] = value;
        }
    }

    void *getGlobalValue(const char *key) {
        auto it = global_values->find(key);
        if (global_values->end() != it)
            return it->second;
        return nullptr;
    }
}