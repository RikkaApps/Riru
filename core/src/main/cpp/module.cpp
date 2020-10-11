#include <dirent.h>
#include <unistd.h>
#include <dlfcn.h>
#include "module.h"
#include "wrap.h"
#include "logging.h"
#include "misc.h"
#include "config.h"
#include "status.h"
#include "hide_utils.h"

std::vector<RiruModule *> *get_modules() {
    static auto *modules = new std::vector<RiruModule *>({new RiruModule(strdup(MODULE_NAME_CORE))});
    return modules;
}

static void *load_module_info_v9(uint32_t token, RiruInit_t *init) {
    auto riru = new Riru();
    riru->token = token;

    auto funcs = new RiruFuncs();
    funcs->getFunc = riru_get_func;
    funcs->setFunc = riru_set_func;
    funcs->getJNINativeMethodFunc = riru_get_native_method_func;
    funcs->setJNINativeMethodFunc = riru_set_native_method_func;
    funcs->getOriginalJNINativeMethodFunc = riru_get_original_native_methods;
    riru->funcs = funcs;

    init(riru);

    return riru->module;
}

void load_modules() {
    DIR *dir;
    struct dirent *entry;
    char path[PATH_MAX];
    void *handle;
    const int riruApiVersion = RIRU_API_VERSION;

    if (!(dir = _opendir(MODULES_DIR))) return;

    while ((entry = _readdir(dir))) {
        if (entry->d_type != DT_DIR) continue;

        auto name = entry->d_name;
        if (name[0] == '.') continue;

        snprintf(path, PATH_MAX, MODULE_PATH_FMT, name);

        if (access(path, F_OK) != 0) {
            PLOGE("access %s", path);
            continue;
        }

        handle = dlopen(path, 0);
        if (!handle) {
            LOGE("dlopen %s failed: %s", path, dlerror());
            continue;
        }

        auto init = (RiruInit_t *) dlsym(handle, "init");
        if (!init) {
            LOGW("%s does not export init", path);
            dlclose(handle);
            continue;
        }

        // 1. pass riru api version, return module's api version
        auto apiVersion = *(int *) init((void *) &riruApiVersion);

        // 2. create and pass Riru struct by module's api version
        auto module = new RiruModule(strdup(name));
        module->handle = handle;
        module->apiVersion = apiVersion;

        if (apiVersion == 9) {
            auto info = load_module_info_v9(module->token, init);
            module->info((RiruModuleInfoV9 *) info);
        } else {
            LOGW("unsupported API %s: %d", name, apiVersion);
            delete module;
            dlclose(handle);
            continue;
        }

        get_modules()->push_back(module);

        LOGI("module loaded: %s (api %d)", module->name, module->apiVersion);
    }

    closedir(dir);

    status::getStatus()->hideEnabled = access(ENABLE_HIDE_FILE, F_OK) == 0;
    if (status::getStatus()->hideEnabled) {
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
        PLOGE("access " ENABLE_HIDE_FILE);
        LOGI("hide is not enabled");
    }

    for (auto module : *get_modules()) {
        if (module->hasOnModuleLoaded()) {
            LOGV("%s: onModuleLoaded", module->name);

            module->onModuleLoaded();
        }
    }
}