#define VERSION 8

extern "C" {
__attribute__((visibility("default"))) int riru_get_version(void) {
    return VERSION;
}
}