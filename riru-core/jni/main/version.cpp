#include "api.h"

#define VERSION 20

extern "C" {
int riru_get_version(void) {
    return VERSION;
}
}