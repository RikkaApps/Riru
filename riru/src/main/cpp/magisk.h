#pragma once

#include <functional>

namespace Magisk {

    const char* GetPath();

    std::string GetPathForSelf(const char *name);

    void ForEachRiruModuleLibrary(const std::function<void(const char* id, const char* riru_files_path)> &fn);
}