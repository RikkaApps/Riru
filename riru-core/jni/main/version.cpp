#include "api.h"

#define VERSION 14

extern "C" {
int riru_get_version(void) {
    return VERSION;
}
}