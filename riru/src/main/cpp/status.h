#pragma once

#include <vector>

namespace Status {

    enum Method {
        forkAndSpecialize = 0,
        forkSystemServer,
        specializeAppProcess,
        COUNT
    };

    void Read(uint8_t *&buffer, uint32_t &buffer_size);

    void WriteSelfAndModules();

    void WriteMethod(Method method, bool replaced, const char *sig);
}
