#include "api.h"

#define VERSION 17

extern "C" {
int riru_get_version(void) {
    return VERSION;
}
}