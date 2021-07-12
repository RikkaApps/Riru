#include <dirent.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <android_prop.h>
#include <memory>
#include <rirud.h>
#include "module.h"
#include "wrap.h"
#include "logging.h"
#include "misc.h"
#include "config.h"
#include "hide_utils.h"
#include "magisk.h"
#include "dl.h"

using namespace std;

std::vector<RiruModule *> &Modules::Get() {
    static auto modules = std::vector<RiruModule *>({new RiruModule(strdup(MODULE_NAME_CORE), "", "")});
    return modules;
}

static void Cleanup(void *handle) {
    if (dlclose(handle) != 0) {
        LOGE("dlclose failed: %s", dlerror());
        return;
    }
}

typedef struct {
    uint32_t token;
    void *getFunc;
    void *getJNINativeMethodFunc;
    void *setFunc;
    void *setJNINativeMethodFunc;
    void *getOriginalJNINativeMethodFunc;
    void *getGlobalValue;
    void *putGlobalValue;
} LegacyApiStub;

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

static void LoadModule(const char *id, const char *path, const char *magisk_module_path) {
    char *name = strdup(id);

    if (access(path, F_OK) != 0) {
        PLOGE("access %s", path);
        return;
    }

    auto handle = dlopen_ext(path, 0);
    if (!handle) {
        LOGE("dlopen %s failed: %s", path, dlerror());
        return;
    }

    auto init = (RiruInit_t *) dlsym(handle, "init");
    if (!init) {
        LOGW("%s does not export init", path);
        Cleanup(handle);
        return;
    }

    auto token = (uintptr_t) name;
    auto legacyApiStub = new LegacyApiStub{
            .token = (uint32_t) token,
            .getFunc = (void *) LegacyApiStubs::getFunc,
            .getJNINativeMethodFunc = (void *) LegacyApiStubs::getNativeMethodFunc,
            .setFunc = (void *) LegacyApiStubs::setFunc,
            .setJNINativeMethodFunc = (void *) LegacyApiStubs::setNativeMethodFunc,
            .getOriginalJNINativeMethodFunc = (void *) LegacyApiStubs::getOriginalNativeMethod,
            .getGlobalValue = (void *) LegacyApiStubs::getGlobalValue,
            .putGlobalValue = (void *) LegacyApiStubs::putGlobalValue
    };

    auto allowUnload = std::make_unique<int>(0);
    auto riru = new Riru{
            .riruApiVersion = riru::apiVersion,
            .unused = (void *) legacyApiStub,
            .magiskModulePath = magisk_module_path,
            .allowUnload = allowUnload.get()
    };

    auto moduleInfo = init(riru);
    if (moduleInfo == nullptr) {
        LOGE("%s requires higher Riru version (or its broken)", path);
        Cleanup(handle);
        return;
    }

    auto apiVersion = moduleInfo->moduleApiVersion;
    if (apiVersion < riru::minApiVersion || apiVersion > riru::apiVersion) {
        LOGW("unsupported API %s: %d", name, apiVersion);
        Cleanup(handle);
        return;
    }

    auto module = new RiruModule(name, strdup(path), strdup(magisk_module_path), token, std::move(allowUnload));
    module->handle = handle;
    module->apiVersion = apiVersion;

    if (apiVersion >= 24) {
        module->info(&moduleInfo->moduleInfo);
    } else {
        moduleInfo = init((Riru *) legacyApiStub);
        if (moduleInfo == nullptr) {
            LOGE("%s returns null on step 2", path);
            Cleanup(handle);
            return;
        }
        module->info((RiruModuleInfo *) moduleInfo);
        init(nullptr);
    }

    Modules::Get().push_back(module);

    LOGI("module loaded: %s (api %d)", module->id, module->apiVersion);
}

void Modules::Load() {
    Magisk::ForEachModule([](const char *path) {
        auto magisk_module_name = basename(path);
        char buf[PATH_MAX];
        DIR *dir;
        struct dirent *entry;

        strcpy(buf, path);
        strcat(buf, "/riru/lib");
#ifdef __LP64__
        strcat(buf, "64");
#endif

        if (access(buf, F_OK) == -1) {
            return;
        }

        LOGI("Magisk module %s is a Riru module", magisk_module_name);

        if (!(dir = opendir(buf))) return;

        strcat(buf, "/");

        while ((entry = readdir(dir))) {
            if (entry->d_type != DT_REG) continue;

            auto end = buf + strlen(buf);
            strcat(buf, entry->d_name);

            char id[PATH_MAX]{0};
            strcpy(id, magisk_module_name);
            strcat(id, "@");

            // remove "lib" or "libriru_"
            if (strncmp(entry->d_name, "libriru_", 8) == 0) {
                strcat(id, entry->d_name + 8);
            } else if (strncmp(entry->d_name, "lib", 3) == 0) {
                strcat(id, entry->d_name + 3);
            } else {
                strcat(id, entry->d_name);
            }

            // remove ".so"
            id[strlen(id) - 3] = '\0';

            LoadModule(id, buf, path);

            *end = '\0';
        }

        closedir(dir);
    });

    std::vector<std::string> dirs;
    if (rirud::ReadDir("/data/adb/riru/modules", dirs)) {
        for (const auto &it : dirs) {
            char path[PATH_MAX];
            auto name = it.c_str();
#ifdef __LP64__
            snprintf(path, PATH_MAX, "/system/lib64/libriru_%s.so", name);
#else
            snprintf(path, PATH_MAX, "/system/lib/libriru_%s.so", name);

#endif
            LoadModule(name, path, "");
        }
    }

    // On Android 10+, zygote has "execmem" permission, we can use "riru hide" here
    if (AndroidProp::GetApiLevel() >= 29) {
        Hide::HideFromMaps();
    }

    for (auto module : Modules::Get()) {
        if (module->hasOnModuleLoaded()) {
            LOGV("%s: onModuleLoaded", module->id);

            module->onModuleLoaded();
        }
    }
}
