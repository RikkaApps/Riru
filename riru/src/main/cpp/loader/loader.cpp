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
#include "misc.h"
#include "buff_string.h"

#ifdef __LP64__
#define LIB_PATH "/system/lib64/"
#else
#define LIB_PATH "/system/lib/"
#endif

#ifdef DEBUG
#ifndef HAS_NATIVE_BRIDGE
#define HAS_NATIVE_BRIDGE
#endif
#endif

#ifdef HAS_NATIVE_BRIDGE

#include "native_bridge_23.h"
#include "native_bridge_24.h"
#include "native_bridge_25.h"
#include "native_bridge_26.h"
#include "native_bridge_27.h"
#include "native_bridge_28.h"
#include "native_bridge_29.h"
#include "native_bridge_30.h"

//NOLINTNEXTLINE
extern "C" [[gnu::visibility("default")]] uint8_t NativeBridgeItf[
        sizeof(android30::NativeBridgeCallbacks) * 2]{0};

static void *original_bridge = nullptr;

__used __attribute__((destructor)) void Destructor() {
    if (original_bridge) dlclose(original_bridge);
}

#endif

__used __attribute__((constructor)) void Constructor() {
    if (getuid() != 0)
        return;

    char cmdline[ARG_MAX + 1];
    get_self_cmdline(cmdline, 0);

    if (strcmp(cmdline, "zygote") != 0
        && strcmp(cmdline, "zygote32") != 0
        && strcmp(cmdline, "zygote64") != 0
        && strcmp(cmdline, "usap32") != 0
        && strcmp(cmdline, "usap64") != 0) {
        LOGW("not zygote (cmdline=%s)", cmdline);
        return;
    }

    LOGI("Riru %s (%d) in %s", riru::versionName, riru::versionCode, cmdline);
    LOGI("Android %s (api %d, preview_api %d)", AndroidProp::GetRelease(),
         AndroidProp::GetApiLevel(),
         AndroidProp::GetPreviewApiLevel());

    int retry = 10;

    RirudSocket rirud;

    std::string magisk_path;
    while (retry > 0) {
        magisk_path = rirud.ReadMagiskTmpfsPath();
        if (!magisk_path.empty()) {
            LOGI("Magisk tmpfs path is %s", magisk_path.data());
            break;
        }
        retry--;
        LOGI("Failed to read Magisk tmpfs path from socket, %d retires left...", retry);
        sleep(1);
    }

    if (magisk_path.empty()) {
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
        auto init = reinterpret_cast<void (*)(void *, const char *, const RirudSocket&)>(dlsym(handle, "init"));
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
        callbacks_size = sizeof(android30::NativeBridgeCallbacks);
    } else if (sdk == __ANDROID_API_Q__) {
        callbacks_size = sizeof(android29::NativeBridgeCallbacks);
    } else if (sdk == __ANDROID_API_P__) {
        callbacks_size = sizeof(android28::NativeBridgeCallbacks);
    } else if (sdk == __ANDROID_API_O_MR1__) {
        callbacks_size = sizeof(android27::NativeBridgeCallbacks);
    } else if (sdk == __ANDROID_API_O__) {
        callbacks_size = sizeof(android26::NativeBridgeCallbacks);
    } else if (sdk == __ANDROID_API_N_MR1__) {
        callbacks_size = sizeof(android25::NativeBridgeCallbacks);
    } else if (sdk == __ANDROID_API_N__) {
        callbacks_size = sizeof(android24::NativeBridgeCallbacks);
    } else if (sdk == __ANDROID_API_M__) {
        callbacks_size = sizeof(android23::NativeBridgeCallbacks);
    }

    memcpy(NativeBridgeItf, original_native_bridge_itf, callbacks_size);
#endif
}
