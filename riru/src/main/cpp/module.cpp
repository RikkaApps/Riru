#include <dirent.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/mman.h>
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
    static auto *modules = new std::vector<RiruModule *>({new RiruModule(strdup(MODULE_NAME_CORE))});
    return modules;
}

static RiruModuleInfoV9 *init_module_v9(uint32_t token, RiruInit_t *init) {
    auto riru = new RiruApiV9();
    riru->token = token;
    riru->getFunc = api::getFunc;
    riru->setFunc = api::setFunc;
    riru->getJNINativeMethodFunc = api::getNativeMethodFunc;
    riru->setJNINativeMethodFunc = api::setNativeMethodFunc;
    riru->getOriginalJNINativeMethodFunc = api::getOriginalNativeMethod;
    riru->getGlobalValue = api::getGlobalValue;
    riru->putGlobalValue = api::putGlobalValue;

    return (RiruModuleInfoV9 *) init(riru);
}

static void cleanup(void *handle, const char *path) {
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

static void load_module(string_view id, const char *path) {
    char *name = (char *) malloc(id.size() + 1);
    name[id.size()] = 0;
    memcpy(name, id.data(), id.size());

    const int riruApiVersion = RIRU_API_VERSION;

    void *handle;

    if (access(path, F_OK) != 0) {
        PLOGE("access %s", path);
        return;
    }

    handle = dl_dlopen(path, 0);
    if (!handle) {
        LOGE("dlopen %s failed: %s", path, dlerror());
        return;
    }

    auto init = (RiruInit_t *) dlsym(handle, "init");
    if (!init) {
        LOGW("%s does not export init", path);
        cleanup(handle, path);
        return;
    }

    // 1. pass riru api version, return module's api version
    auto apiVersion = (int *) init((void *) &riruApiVersion);
    if (apiVersion == nullptr) {
        LOGE("%s returns null on step 1", path);
        cleanup(handle, path);
        return;
    }

    if (*apiVersion < RIRU_MIN_API_VERSION || *apiVersion > RIRU_API_VERSION) {
        LOGW("unsupported API %s: %d", name, *apiVersion);
        cleanup(handle, path);
        return;
    }

    // 2. create and pass Riru struct by module's api version
    auto module = new RiruModule(name);
    module->handle = handle;
    module->apiVersion = *apiVersion;

    if (*apiVersion == 10 || *apiVersion == 9) {
        auto info = init_module_v9(module->token, init);
        if (info == nullptr) {
            LOGE("%s returns null on step 2", path);
            cleanup(handle, path);
            return;
        }
        module->info(info);
    }

    // 3. let the module to do some cleanup jobs
    init(nullptr);

    get_modules()->push_back(module);

    LOGI("module loaded: %s (api %d)", module->name, module->apiVersion);
}

void load_modules() {
    uint8_t *buffer;
    uint32_t buffer_size;

    Magisk::ForEachRiruModuleLibrary([](std::string_view id, const char *path) {
        load_module(id, path);
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

    hide_enabled = status->core()->hide();

    for (auto it : *status->modules()) {
        char path[PATH_MAX];
        auto name = it->name()->c_str();
        snprintf(path, PATH_MAX, MODULE_PATH_FMT, name);

        load_module(name, path);
    }

    if (hide_enabled) {
        LOGI("hide is enabled");
        auto modules = get_modules();
        auto names = (const char **) malloc(sizeof(char *) * modules->size());
        int names_count = 0;
        for (auto module : *get_modules()) {
            if (strcmp(module->name, MODULE_NAME_CORE) == 0) continue;
            if (!module->supportHide) {
                LOGI("module %s does not support hide", module->name);
                continue;
            }
            names[names_count] = module->name;
            names_count += 1;
        }
        hide::hide_modules(names, names_count);
    } else {
        LOGI("hide is not enabled");
    }

    for (auto module : *get_modules()) {
        if (module->hasOnModuleLoaded()) {
            LOGV("%s: onModuleLoaded", module->name);

            module->onModuleLoaded();
        }
    }

    clean:
    free(buffer);
}