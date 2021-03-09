#pragma once

#include <functional>

namespace Magisk {

    const char* GetPath();

    std::string GetPathForSelf(const char *name);

    void ForEachModule(const std::function<void(const char* riru_files_path)> &fn);
}