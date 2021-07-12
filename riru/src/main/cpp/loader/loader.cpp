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
#include <rirud.h>
#include "config.h"
#include "logging.h"
#include "misc.h"

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

static const uint32_t ACTION_READ_NATIVE_BRIDGE = 3;
static const uint32_t ACTION_READ_MAGISK_TMPFS_PATH = 6;

#ifdef HAS_NATIVE_BRIDGE

#include "native_bridge_23.h"
#include "native_bridge_24.h"
#include "native_bridge_25.h"
#include "native_bridge_26.h"
#include "native_bridge_27.h"
#include "native_bridge_28.h"
#include "native_bridge_29.h"
#include "native_bridge_30.h"

extern "C" __used __attribute__((visibility("default"))) uint8_t NativeBridgeItf[sizeof(android30::NativeBridgeCallbacks) * 2]{0};

static void *original_bridge = nullptr;

__used __attribute__((destructor)) void destructor() {
    if (original_bridge) dlclose(original_bridge);
}

#endif

static void ReadFromSocket(uint32_t code, char *buffer, int32_t &buffer_size) {
    struct sockaddr_un addr{};
    int fd;
    socklen_t socklen;

    if ((fd = socket(PF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0) {
        PLOGE("socket");
        goto clean;
    }

    socklen = setup_sockaddr(&addr, SOCKET_ADDRESS);

    if (connect(fd, (struct sockaddr *) &addr, socklen) == -1) {
        PLOGE("connect %s", SOCKET_ADDRESS);
        goto clean;
    }

    if (write_full(fd, &code, sizeof(code)) != 0) {
        PLOGE("write %s", SOCKET_ADDRESS);
        goto clean;
    }

    if (read_full(fd, &buffer_size, sizeof(buffer_size)) != 0) {
        PLOGE("read %s", SOCKET_ADDRESS);
        goto clean;
    }

    LOGD("size=%d", buffer_size);

    if (buffer_size > 0 && buffer_size < PATH_MAX) {
        if (read_full(fd, buffer, buffer_size) != 0) {
            PLOGE("read %s", SOCKET_ADDRESS);
            goto clean;
        }
    }

    clean:
    if (fd != -1) close(fd);
}

__used __attribute__((constructor)) void constructor() {
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
    LOGI("Android %s (api %d, preview_api %d)", AndroidProp::GetRelease(), AndroidProp::GetApiLevel(),
         AndroidProp::GetPreviewApiLevel());

    char magisk_path[PATH_MAX]{0};
    int32_t buf_size = -1;
    int retry = 10;

    while (retry > 0) {
        ReadFromSocket(ACTION_READ_MAGISK_TMPFS_PATH, magisk_path, buf_size);
        if (buf_size > 0) {
            LOGI("Magisk tmpfs path is %s", magisk_path);
            break;
        }
        retry --;
        LOGI("Failed to read Magisk tmpfs path from socket, %d retires left...", retry);
        sleep(1);
    }

    if (buf_size <= 0) {
        return;
    }

    char riru_path[PATH_MAX];
    strcpy(riru_path, magisk_path);
    strcat(riru_path, "/.magisk/modules/riru-core/lib");
#ifdef __LP64__
    strcat(riru_path, "64");
#endif
    strcat(riru_path, "/libriru.so");

    auto handle = dlopen_ext(riru_path, 0);
    if (handle) {
        auto init = (void(*)(void *, const char*)) dlsym(handle, "init");
        if (init) {
            init(handle, magisk_path);
        } else {
            LOGE("dlsym init %s", dlerror());
        }
    } else {
        LOGE("dlopen riru.so %s", dlerror());
    }

#ifdef HAS_NATIVE_BRIDGE

    char native_bridge[PATH_MAX]{0};

    ReadFromSocket(ACTION_READ_NATIVE_BRIDGE, native_bridge, buf_size);
    if (buf_size <= 0) {
        LOGW("Failed to read original native bridge from socket");
        return;
    }

    LOGI("original native bridge: %s", native_bridge);

    if (native_bridge[0] == '0' && native_bridge[1] == '\0') {
        return;
    }

    original_bridge = dlopen(native_bridge, RTLD_NOW);
    if (original_bridge == nullptr) {
        LOGE("dlopen failed: %s", dlerror());
        return;
    }

    auto original_NativeBridgeItf = dlsym(original_bridge, "NativeBridgeItf");
    if (original_NativeBridgeItf == nullptr) {
        LOGE("dlsym failed: %s", dlerror());
        return;
    }

    int sdk = 0;
    char value[PROP_VALUE_MAX + 1];
    if (__system_property_get("ro.build.version.sdk", value) > 0)
        sdk = atoi(value);

    auto callbacks_size = 0;
    if (sdk >= 30) {
        callbacks_size = sizeof(android30::NativeBridgeCallbacks);
    } else if (sdk == 29) {
        callbacks_size = sizeof(android29::NativeBridgeCallbacks);
    } else if (sdk == 28) {
        callbacks_size = sizeof(android28::NativeBridgeCallbacks);
    } else if (sdk == 27) {
        callbacks_size = sizeof(android27::NativeBridgeCallbacks);
    } else if (sdk == 26) {
        callbacks_size = sizeof(android26::NativeBridgeCallbacks);
    } else if (sdk == 25) {
        callbacks_size = sizeof(android25::NativeBridgeCallbacks);
    } else if (sdk == 24) {
        callbacks_size = sizeof(android24::NativeBridgeCallbacks);
    } else if (sdk == 23) {
        callbacks_size = sizeof(android23::NativeBridgeCallbacks);
    }

    memcpy(NativeBridgeItf, original_NativeBridgeItf, callbacks_size);
#endif
}
