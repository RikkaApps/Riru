#ifndef RIRUD_H
#define RIRUD_H

#include <cstdint>
#include <vector>
#include <string>

namespace rirud {

    const uint32_t ACTION_READ_FILE = 4;
    const uint32_t ACTION_READ_DIR = 5;

    // used by riru itself only, could be removed in the future
    const uint32_t ACTION_WRITE_STATUS = 2;
    const uint32_t ACTION_READ_NATIVE_BRIDGE = 3;
    const uint32_t ACTION_READ_MAGISK_TMPFS_PATH = 6;

    const uint8_t CODE_OK = 0;
    const uint8_t CODE_FAILED = 1;

    int OpenSocket();

    void CloseSocket();

    ssize_t ReadMagiskTmpfsPath(char *buffer);

    bool ReadDir(const char *path, std::vector<std::string> &dirs);

    struct Module {
        char id[256]{0};
        uint32_t apiVersion = 0;
        uint32_t version = 0;
        char versionName[256]{0};
        uint8_t supportHide = 0;
    };

    void WriteModules(const std::vector<Module> &modules);
}

#endif //RIRUD_H
