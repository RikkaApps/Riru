#pragma once

#include <status_generated.h>

namespace Status {

    const uint32_t ACTION_PING = 0;
    const uint32_t ACTION_READ = 1;
    const uint32_t ACTION_WRITE = 2;
    const uint32_t ACTION_READ_NATIVE_BRIDGE = 3;

    const uint8_t CODE_OK = 0;
    const uint8_t CODE_FAILED = 1;

    void WriteToFile(const FbStatus *status);

    void ReadFromFile(flatbuffers::FlatBufferBuilder &builder);
}
