#define VERSION 12

extern "C" {
__attribute__((visibility("default"))) int riru_get_version(void) {
    return VERSION;
}
}