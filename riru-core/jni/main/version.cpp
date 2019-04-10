#include "api.h"

#define VERSION 19

extern "C" {
int riru_get_version(void) {
    return VERSION;
}
}