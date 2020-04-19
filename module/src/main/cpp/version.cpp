#include "api.h"

extern "C" {
int riru_get_version(void) {
    return RIRU_VERSION_CODE;
}
}