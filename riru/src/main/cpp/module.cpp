#include <dirent.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <android_prop.h>
#include <memory>
#include "buff_string.h"
#include <rirud.h>
#include "module.h"
#include "wrap.h"
#include "logging.h"
#include "misc.h"
#include "config.h"
#include "hide_utils.h"
#include "magisk.h"
#include "dl.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

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
    constexpr uint8_t is64bit = sizeof(void *) == 8;
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
        rirud.Write<int32_t>(module.supportHide);
    }

}

void modules::Load(const RirudSocket &rirud) {
    magisk::ForEachModule([](const char *path) {
        const auto *magisk_module_name = basename(path);
        BuffString<PATH_MAX> buf;
        DIR *dir;
        struct dirent *entry;

        buf += path;
        buf += "/riru/lib";
#ifdef __LP64__
        buf += "64";
#endif

        if (access(buf, F_OK) == -1) {
            return;
        }

        LOGI("Magisk module %s is a Riru module", magisk_module_name);

        if (!(dir = opendir(buf))) return;

        buf += "/";

        auto end = buf.size();

        while ((entry = readdir(dir))) {
            if (entry->d_type != DT_REG) continue;

            buf += entry->d_name;

            BuffString<PATH_MAX> id;
            id += magisk_module_name;
            id += "@";

            constexpr auto libriru = "libriru_"sv;
            constexpr auto lib = "lib"sv;
            // remove "lib" or "libriru_"
            std::string_view d_name(entry->d_name);
            if (d_name.substr(0, libriru.size()) == libriru) {
                id += entry->d_name + libriru.size();
            } else if (d_name.substr(0, lib.size()) == lib) {
                id += entry->d_name + lib.size();
            } else {
                id += entry->d_name;
            }

            // remove ".so"
            id.size(id.size() - 3);

            LoadModule(id, buf, path);

            buf.size(end);
        }

        closedir(dir);
    });

    std::vector<std::string> dirs;
    BuffString<PATH_MAX> path;

#ifdef __LP64__
    path += "/system/lib64/libriru_";
#else
    path += "/system/lib/libriru_";
#endif

    auto end = path.size();
    for (auto iter = rirud.ReadDir("/data/adb/riru/modules"); iter; ++iter) {
        path += *iter;
        path += ".so";
        LoadModule(*iter, path, "");
        path.size(end);
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
