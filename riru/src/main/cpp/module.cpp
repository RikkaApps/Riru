#include <dirent.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <android_prop.h>
#include <memory>
#include "buff_string.h"
#include <rirud.h>
#include "module.h"
#include "logging.h"
#include "config.h"
#include "hide_utils.h"
#include "magisk.h"
#include "dl.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

constexpr uint8_t is64bit = sizeof(void *) == 8;

std::list<RiruModule> &modules::Get() {
    static std::list<RiruModule> kModules;
    return kModules;
}

static void Cleanup(void *handle) {
    if (dlclose(handle) != 0) {
        LOGE("dlclose failed: %s", dlerror());
        return;
    }
}

static void
LoadModule(std::string_view id, std::string_view path, std::string_view magisk_module_path) {
    if (access(path.data(), F_OK) != 0) {
        PLOGE("access %s", path.data());
        return;
    }

    auto *handle = DlopenExt(path.data(), 0);
    if (!handle) {
        LOGE("dlopen %s failed: %s", path.data(), dlerror());
        return;
    }

    auto init = reinterpret_cast<RiruInit_t *>(dlsym(handle, "init"));
    if (!init) {
        LOGW("%s does not export init", path.data());
        Cleanup(handle);
        return;
    }

    auto allow_unload = std::make_unique<int>();
    auto riru = std::make_unique<Riru>(Riru{
            .riruApiVersion = riru::apiVersion,
            .unused = nullptr,
            .magiskModulePath = magisk_module_path.data(),
            .allowUnload = allow_unload.get()
    });

    auto *module_info = init(riru.get());
    if (module_info == nullptr) {
        LOGE("%s requires higher Riru version (or its broken)", path.data());
        Cleanup(handle);
        return;
    }

    auto api_version = module_info->moduleApiVersion;
    if (api_version < riru::minApiVersion || api_version > riru::apiVersion) {
        LOGW("unsupported API %s: %d", id.data(), api_version);
        Cleanup(handle);
        return;
    }

    LOGI("module loaded: %s (api %d)", id.data(), api_version);

    modules::Get().emplace_back(id, path, magisk_module_path, api_version, module_info->moduleInfo,
                                handle,
                                std::move(allow_unload));
}

static void WriteModules(const RirudSocket &rirud) {
    auto &modules = modules::Get();
    uint32_t count = modules.size();
    if (!rirud.Write(RirudSocket::Action::WRITE_STATUS) || !rirud.Write(is64bit) ||
        !rirud.Write(count)) {
        PLOGE("write %s", SOCKET_ADDRESS);
        return;
    }

    for (const auto &module : modules) {
        rirud.Write(module.id);
        rirud.Write<int32_t>(module.apiVersion);
        rirud.Write<int32_t>(module.version);
        rirud.Write(module.versionName);
        rirud.Write<int8_t>(module.supportHide);
    }
}

void modules::Load(const RirudSocket &rirud) {
    uint32_t num_modules;
    auto &modules = modules::Get();
    if (!rirud.Write(RirudSocket::Action::READ_MODULES) ||
        !rirud.Write(is64bit) || !rirud.Read(num_modules)) {
        LOGE("Faild to load modules");
        return;
    }
    std::string magisk_module_path;
    std::string path;
    std::string id;
    uint32_t num_libs;
    while (num_modules-- > 0) {
        if (!rirud.Read(magisk_module_path) || !rirud.Read(num_libs)) {
            LOGE("Faild to read module's magisk path");
            return;
        }
        while (num_libs-- > 0) {
            if (!rirud.Read(id) || !rirud.Read(path)) {
                LOGE("Faild to read module's lib path");
                return;
            }
            LoadModule(id, path, magisk_module_path);
        }
    }

    // On Android 10+, zygote has "execmem" permission, we can use "riru hide" here
    if (AndroidProp::GetApiLevel() >= __ANDROID_API_Q__) {
        hide::HideFromMaps();
    }

    for (const auto &module : modules::Get()) {
        if (module.hasOnModuleLoaded()) {
            LOGV("%s: onModuleLoaded", module.id.data());
            module.onModuleLoaded();
        }
    }

    WriteModules(rirud);
}
