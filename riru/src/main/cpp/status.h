#pragma once

#include <vector>
#include "module.h"

namespace Status {

    enum Method {
        forkAndSpecialize = 0,
        forkSystemServer,
        specializeAppProcess,
        COUNT
    };

    void Write();

    void WriteMethod(Method method, bool replaced, const char *sig);
}
