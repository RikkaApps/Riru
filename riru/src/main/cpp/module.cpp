#include <dirent.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <android_prop.h>
#include "module.h"
#include "wrap.h"
#include "logging.h"
#include "misc.h"
#include "config.h"
#include "status.h"
#include "hide_utils.h"
#include "status_generated.h"
#include "magisk.h"
#include "dl.h"

using namespace std;

static bool hide_enabled;

bool is_hide_enabled() {
    return hide_enabled;
}

std::vector<RiruModule *> *get_modules() {
    static auto *modules = new std::vector<RiruModule *>({new RiruModule(strdup(MODULE_NAME_CORE), "", "")});
    return modules;
}

static void Cleanup(void *handle, const char *path) {
    if (dlclose(handle) != 0) {
        LOGE("dlclose failed: %s", dlerror());
        return;
    }

    procmaps_iterator *maps = pmparser_parse(-1);
    if (maps == nullptr) {
        LOGE("cannot parse the memory map");
        return;
    }

    procmaps_struct *maps_tmp;
    while ((maps_tmp = pmparser_next(maps)) != nullptr) {
        if (strcmp(maps_tmp->pathname, path) != 0) continue;

        auto start = (uintptr_t) maps_tmp->addr_start;
        auto end = (uintptr_t) maps_tmp->addr_end;
        auto size = end - start;
        LOGD("%" PRIxPTR"-%" PRIxPTR" %s %ld %s", start, end, maps_tmp->perm, maps_tmp->offset, maps_tmp->pathname);
        munmap((void *) start, size);
    }
    pmparser_free(maps);
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
        Cleanup(handle, path);
        return;
    }

    auto token = (uintptr_t) name;
    auto riruApi = new RiruApi();
    riruApi->token = token;
    riruApi->getFunc = api::getFunc;
    riruApi->setFunc = api::setFunc;
    riruApi->getJNINativeMethodFunc = api::getNativeMethodFunc;
    riruApi->setJNINativeMethodFunc = api::setNativeMethodFunc;
    riruApi->getOriginalJNINativeMethodFunc = api::getOriginalNativeMethod;
    riruApi->getGlobalValue = api::getGlobalValue;
    riruApi->putGlobalValue = api::putGlobalValue;

    auto riru = new Riru();
    riru->riruApiVersion = RIRU_API_VERSION;
    riru->riruApi = riruApi;
    riru->magiskModulePath = magisk_module_path;

    auto moduleInfo = init(riru);
    if (moduleInfo == nullptr) {
        LOGE("%s requires higher Riru version (or its broken)", path);
        Cleanup(handle, path);
        return;
    }

    auto apiVersion = moduleInfo->moduleApiVersion;
    if (apiVersion < RIRU_MIN_API_VERSION || apiVersion > RIRU_API_VERSION) {
        LOGW("unsupported API %s: %d", name, apiVersion);
        Cleanup(handle, path);
        return;
    }

    auto module = new RiruModule(name, strdup(path), strdup(magisk_module_path), token);
    module->handle = handle;
    module->apiVersion = apiVersion;

    if (apiVersion >= 24) {
        module->info(&moduleInfo->moduleInfo);
    } else {
        moduleInfo = init((Riru *) riruApi);
        if (moduleInfo == nullptr) {
            LOGE("%s returns null on step 2", path);
            Cleanup(handle, path);
            return;
        }
        module->info((RiruModuleInfo *) moduleInfo);
        init(nullptr);
    }

    get_modules()->push_back(module);

    LOGI("module loaded: %s (api %d)", module->id, module->apiVersion);
}

void Modules::Load() {
    uint8_t *buffer;
    uint32_t buffer_size;

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

    if (!Status::ReadModules(buffer, buffer_size)) {
        return;
    }

    auto status = Status::GetFbStatus(buffer);

    if (!status->core()) {
        LOGW("core is null");
        goto clean;
    }
    if (!status->modules()) {
        LOGW("modules is null");
        goto clean;
    }

    for (auto it : *status->modules()) {
        char path[PATH_MAX];
        auto name = it->name()->c_str();
#ifdef __LP64__
        snprintf(path, PATH_MAX, "/system/lib64/libriru_%s.so", name);
#else
        snprintf(path, PATH_MAX, "/system/lib/libriru_%s.so", name);

#endif
        LoadModule(name, path, "");
    }

    Hide::DoHide(true, AndroidProp::GetApiLevel() >= 29);

    for (auto module : *get_modules()) {
        if (module->hasOnModuleLoaded()) {
            LOGV("%s: onModuleLoaded", module->id);

            module->onModuleLoaded();
        }
    }

    clean:
    free(buffer);
}