#include "api.h"

#define VERSION 15

extern "C" {
int riru_get_version(void) {
    return VERSION;
}
}