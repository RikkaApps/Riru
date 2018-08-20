#define VERSION 2

extern "C" {
__attribute__((visibility("default"))) int get_version(void) {
    return VERSION;
}
}