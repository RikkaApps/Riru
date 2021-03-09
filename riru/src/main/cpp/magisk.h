#pragma once

#include <functional>

namespace Magisk {

    const char* GetPath();

    std::string GetPathForSelf(const char *name);

    void ForEachRiruModuleLibrary(const std::function<void(std::string_view id, const char* riru_files_path)> &fn);
}