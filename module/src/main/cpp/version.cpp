#include "api.h"
#include "version.h"

extern "C" {
int riru_get_version(void) {
    return VERSION_CODE;
}
}