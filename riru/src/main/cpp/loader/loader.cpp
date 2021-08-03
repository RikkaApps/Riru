#include <dlfcn.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <cstdio>
#include <sys/system_properties.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <socket.h>
#include <malloc.h>
#include <dl.h>
#include <android_prop.h>
#include "rirud.h"
#include "config.h"
#include "logging.h"
#include <list>
#include "buff_string.h"

#ifndef NDEBUG
#ifndef HAS_NATIVE_BRIDGE
#define HAS_NATIVE_BRIDGE
#endif
#endif

#ifdef HAS_NATIVE_BRIDGE

#include "native_bridge_callbacks.h"

//NOLINTNEXTLINE
extern "C" [[gnu::visibility("default")]] uint8_t NativeBridgeItf[
        sizeof(NativeBridgeCallbacks<__ANDROID_API_R__>) * 2]{0};

static void *original_bridge = nullptr;

__used __attribute__((destructor)) void Destructor() {
    if (original_bridge) dlclose(original_bridge);
}

#endif

std::list<std::string> GetSelfCmdline() {
    std::list<std::string> cmdlines;

    FILE *f = fopen("/proc/self/cmdline", "rb");

    if (!f) {
        return cmdlines;
    }

    char *line = nullptr;
    size_t len = 0;

    while (getdelim(&line, &len, '\0', f) != -1) {
        cmdlines.emplace_back(line);
    }
    free(line);

    fclose(f);
    return cmdlines;
}

__used __attribute__((constructor)) void Constructor() {
    if (getuid() != 0) {
        return;
    }

    auto cmdlines = GetSelfCmdline();
    if (cmdlines.empty()) {
        LOGW("failed to get cmdline");
        return;
    }
    auto& cmdline = cmdlines.front();

    if (cmdline != "zygote" &&
        cmdline != "zygote32" &&
        cmdline != "zygote64" &&
        cmdline != "usap32" &&
        cmdline != "usap64") {
        LOGW("not zygote (cmdline=%s)", cmdline.data());
        return;
    }

    LOGI("Riru %s (%d) in %s", riru::versionName, riru::versionCode, cmdline.data());
    LOGI("Android %s (api %d, preview_api %d)", AndroidProp::GetRelease(),
         AndroidProp::GetApiLevel(),
         AndroidProp::GetPreviewApiLevel());

    RirudSocket rirud;

    if (!rirud.valid()) {
        LOGE("rirud connect fails");
        return;
    }

    std::string magisk_path = rirud.ReadMagiskTmpfsPath();
    if (magisk_path.empty()) {
        LOGE("failed to obtain magisk path");
        return;
    }

    BuffString<PATH_MAX> riru_path;
    riru_path += magisk_path;
    riru_path += "/.magisk/modules/riru-core/lib";
#ifdef __LP64__
    riru_path += "64";
#endif
    riru_path += "/libriru.so";

    auto *handle = DlopenExt(riru_path, 0);
    if (handle) {
        auto init = reinterpret_cast<void (*)(void *, const char *, const RirudSocket &)>(dlsym(
                handle, "init"));
        if (init) {
            init(handle, magisk_path.data(), rirud);
        } else {
            LOGE("dlsym init %s", dlerror());
        }
    } else {
        LOGE("dlopen riru.so %s", dlerror());
    }

#ifdef HAS_NATIVE_BRIDGE

    auto native_bridge = rirud.ReadNativeBridge();
    if (native_bridge.empty()) {
        LOGW("Failed to read original native bridge from socket");
        return;
    }

    LOGI("original native bridge: %s", native_bridge.data());

    if (native_bridge == "0") {
        return;
    }

    original_bridge = dlopen(native_bridge.data(), RTLD_NOW);
    if (original_bridge == nullptr) {
        LOGE("dlopen failed: %s", dlerror());
        return;
    }

    auto *original_native_bridge_itf = dlsym(original_bridge, "NativeBridgeItf");
    if (original_native_bridge_itf == nullptr) {
        LOGE("dlsym failed: %s", dlerror());
        return;
    }

    int sdk = 0;
    std::array<char, PROP_VALUE_MAX + 1> value;
    if (__system_property_get("ro.build.version.sdk", value.data()) > 0) {
        sdk = atoi(value.data());
    }

    auto callbacks_size = 0;
    if (sdk >= __ANDROID_API_R__) {
        callbacks_size = sizeof(NativeBridgeCallbacks<__ANDROID_API_R__>);
    } else if (sdk == __ANDROID_API_Q__) {
        callbacks_size = sizeof(NativeBridgeCallbacks<__ANDROID_API_Q__>);
    } else if (sdk == __ANDROID_API_P__) {
        callbacks_size = sizeof(NativeBridgeCallbacks<__ANDROID_API_P__>);
    } else if (sdk == __ANDROID_API_O_MR1__) {
        callbacks_size = sizeof(NativeBridgeCallbacks<__ANDROID_API_O_MR1__>);
    } else if (sdk == __ANDROID_API_O__) {
        callbacks_size = sizeof(NativeBridgeCallbacks<__ANDROID_API_O__>);
    } else if (sdk == __ANDROID_API_N_MR1__) {
        callbacks_size = sizeof(NativeBridgeCallbacks<__ANDROID_API_N_MR1__>);
    } else if (sdk == __ANDROID_API_N__) {
        callbacks_size = sizeof(NativeBridgeCallbacks<__ANDROID_API_N__>);
    } else if (sdk == __ANDROID_API_M__) {
        callbacks_size = sizeof(NativeBridgeCallbacks<__ANDROID_API_M__>);
    }

    memcpy(NativeBridgeItf, original_native_bridge_itf, callbacks_size);
#endif
}
