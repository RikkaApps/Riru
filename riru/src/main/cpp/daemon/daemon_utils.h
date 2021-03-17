#pragma once

namespace Daemon {

    int GetMagiskVersion();

    const char *GetMagiskTmpfsPath();

    void GenerateRandomName();

    void WriteToFile(bool is_64bit, uint32_t count, const rirud::Module* modules);
}
