#pragma once

#include <vector>

namespace Status {

    enum Method {
        forkAndSpecialize = 0,
        forkSystemServer,
        specializeAppProcess,
        COUNT
    };

    bool ReadModules(uint8_t *&buffer, uint32_t &buffer_size);

    void WriteSelfAndModules();

    void WriteMethod(Method method, bool replaced, const char *sig);

    bool ReadFile(const char *path, int target_fd);

    void ReadMagiskTmpfsPath(char *&buffer, int32_t &buffer_size);
}
