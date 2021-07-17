#pragma once

#include <jni.h>
#include <dlfcn.h>
#include <string>
#include <list>
#include <riru.h>
#include <memory>
#include "rirud.h"

struct RiruModule : public RiruModuleInfo {

public:
    std::string id;
    std::string path;
    std::string magisk_module_path;
    int apiVersion;


private:
    void *handle_{};
    std::unique_ptr<int> _allowUnload;

public:
    RiruModule(RiruModule &&other) = default;

    explicit RiruModule(std::string_view id, std::string_view path,
                        std::string_view magisk_module_path,
                        int apiVersion, const RiruModuleInfo &info, void *handle = nullptr,
                        std::unique_ptr<int> allowUnload = nullptr) : RiruModuleInfo(info),
                                                                      id(id), path(path),
                                                                      magisk_module_path(
                                                                              magisk_module_path),
                                                                      apiVersion(apiVersion),
                                                                      handle_(handle), _allowUnload(
                    std::move(allowUnload)) {
    }

    void unload() {
        if (!handle_) return;

        if (dlclose(handle_) == 0) {
            handle_ = nullptr;
        }
    }

    bool isLoaded() const {
        return handle_ != nullptr;
    }

    bool allowUnload() const {
        return apiVersion >= 25
               && ((_allowUnload && *_allowUnload != 0) || !hasAppFunctions());
    }

    void resetAllowUnload() const {
        if (_allowUnload) *_allowUnload = 0;
    }

    bool hasOnModuleLoaded() const {
        return onModuleLoaded;
    }

    bool hasShouldSkipUid() const {
        return shouldSkipUid;
    }

    bool hasForkAndSpecializePre() const {
        return forkAndSpecializePre;
    }

    bool hasForkAndSpecializePost() const {
        return forkAndSpecializePost;
    }

    bool hasForkSystemServerPre() const {
        return forkSystemServerPre;
    }

    bool hasForkSystemServerPost() const {
        return forkSystemServerPost;
    }

    bool hasSpecializeAppProcessPre() const {
        return specializeAppProcessPre;
    }

    bool hasSpecializeAppProcessPost() const {
        return specializeAppProcessPost;
    }

    bool hasAppFunctions() const {
        return hasForkAndSpecializePre()
               || hasForkAndSpecializePost()
               || hasSpecializeAppProcessPre()
               || hasSpecializeAppProcessPost();
    }
};

namespace modules {

    std::list<RiruModule> &Get();

    void Load(const RirudSocket &rirud);
}
