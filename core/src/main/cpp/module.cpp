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

std::vector<RiruModuleExt *> *get_modules() {
    static auto *modules = new std::vector<RiruModuleExt *>({new RiruModuleExt(strdup(MODULE_NAME_CORE))});
    return modules;
}

void load_modules() {
    DIR *dir;
    struct dirent *entry;
    char path[PATH_MAX], modules_path[PATH_MAX], module_prop[PATH_MAX], prop_value[PATH_MAX];
    int module_api_version;
    void *handle;

    RiruFuncs riru_funcs;
    riru_funcs.getFunc = riru_get_func;
    riru_funcs.setFunc = riru_set_func;
    riru_funcs.getJNINativeMethodFunc = riru_get_native_method_func;
    riru_funcs.setJNINativeMethodFunc = riru_set_native_method_func;
    riru_funcs.getOriginalJNINativeMethodFunc = riru_get_original_native_methods;

    Riru riru_init_data;
    riru_init_data.version = RIRU_VERSION_CODE;
    riru_init_data.funcs = &riru_funcs;

    snprintf(modules_path, PATH_MAX, "%s/modules", CONFIG_DIR);

    if (!(dir = _opendir(modules_path))) return;

    while ((entry = _readdir(dir))) {
        if (entry->d_type != DT_DIR) continue;

        auto name = entry->d_name;
        if (name[0] == '.') continue;

        snprintf(path, PATH_MAX, MODULE_PATH_FMT, name);

        if (access(path, F_OK) != 0) {
            PLOGE("access %s", path);
            continue;
        }

        snprintf(module_prop, PATH_MAX, "%s/%s/module.prop", modules_path, name);
        if (access(module_prop, F_OK) != 0) {
            PLOGE("access %s", module_prop);
            continue;
        }

        module_api_version = -1;
        if (get_prop(module_prop, "api", prop_value) > 0) {
            module_api_version = atoi(prop_value);
        }
        if (module_api_version < 8) {
            LOGW("module %s does not support Riru v21+", name);
            continue;
        }

        handle = dlopen(path, 0);
        if (!handle) {
            LOGE("dlopen %s failed: %s", path, dlerror());
            continue;
        }

        auto init = (RiruInit_t *) dlsym(handle, "init");
        if (!init) {
            LOGW("module %s does not export init", name);
            dlclose(handle);
            continue;
        }

        auto *module = new RiruModuleExt(strdup(name));

        riru_init_data.module = module;
        riru_init_data.token = module->token;
        init(&riru_init_data);

        module->handle = handle;

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
        if (module->onModuleLoaded) {
            LOGV("%s: onModuleLoaded", module->name);

            module->onModuleLoaded();
        }
    }
}