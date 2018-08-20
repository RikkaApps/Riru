#define VERSION 4

extern "C" {
__attribute__((visibility("default"))) int get_version(void) {
    return VERSION;
}
}