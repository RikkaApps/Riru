#pragma once

namespace status {

    enum method {
        forkAndSpecialize = 0,
        forkSystemServer,
        specializeAppProcess,
        COUNT
    };

    struct status_t {
        const char *methodName[method::COUNT] = {
                "nativeForkAndSpecialize",
                "nativeForkSystemServer",
                "nativeSpecializeAppProcess"
        };
        bool methodReplaced[method::COUNT] = {false, false, false};
        const char *methodSignature[method::COUNT]{"", "", ""};
        bool hideEnabled = false;
    };

    status_t *getStatus();

    void writeToFile();

    void writeMethodToFile(method method, bool replaced, const char *sig);
}
