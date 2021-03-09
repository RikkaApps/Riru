#pragma once

#include <status_generated.h>

namespace Status {

    const uint32_t ACTION_PING = 0;
    const uint32_t ACTION_READ_FILE = 4;
    const uint32_t ACTION_READ_DIR = 5;

    // used by riru itself only, could be removed in the future
    const uint32_t ACTION_READ_STATUS = 1;
    const uint32_t ACTION_WRITE_STATUS = 2;
    const uint32_t ACTION_READ_NATIVE_BRIDGE = 3;
    const uint32_t ACTION_READ_MAGISK_TMPFS_PATH = 6;

    const uint8_t CODE_OK = 0;
    const uint8_t CODE_FAILED = 1;

    int GetMagiskVersion();

    const char *GetMagiskTmpfsPath();

    void GenerateRandomName();

    void WriteToFile(const FbStatus *status);

    void ReadFromFile(flatbuffers::FlatBufferBuilder &builder);
}
