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

static const uint32_t ACTION_PING = 0;
static const uint32_t ACTION_READ_NATIVE_BRIDGE = 3;

static const uint8_t CODE_OK = 0;
static const uint8_t CODE_FAILED = 1;

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

static void ReadOriginalNativeBridgeFromSocket(char *buffer, int32_t &buffer_size) {
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

    if (write_full(fd, &ACTION_READ_NATIVE_BRIDGE, sizeof(ACTION_READ_NATIVE_BRIDGE)) != 0) {
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

#endif

static void WaitForSocket(uint8_t retry) {
    struct sockaddr_un addr{};
    int fd;
    socklen_t socklen;
    uint8_t reply = CODE_FAILED;

    while (retry > 0) {
        if ((fd = socket(PF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0) {
            PLOGE("socket");
            goto clean;
        }

        socklen = setup_sockaddr(&addr, SOCKET_ADDRESS);

        if (connect(fd, (struct sockaddr *) &addr, socklen) == -1) {
            PLOGE("connect %s", SOCKET_ADDRESS);
            goto clean;
        }

        if (write_full(fd, &ACTION_PING, sizeof(ACTION_PING)) != 0) {
            PLOGE("write %s", SOCKET_ADDRESS);
            goto clean;
        }

        if (read_full(fd, &reply, sizeof(reply)) != 0) {
            PLOGE("read %s", SOCKET_ADDRESS);
            goto clean;
        }

        clean:
        if (fd != -1) close(fd);
        retry -= 1;

        if (reply == CODE_OK) {
            LOGI("socket is running!");
            break;
        } else {
            LOGI("socket is not running, %d retries left...", retry);
            sleep(1);
        }
    }
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

    WaitForSocket(10);

    dlopen(LIB_PATH "libriru.so", 0);

#ifdef HAS_NATIVE_BRIDGE

    char buf[PATH_MAX]{0};
    int32_t buf_size;
    ssize_t size;

    ReadOriginalNativeBridgeFromSocket(buf, buf_size);

    if (buf_size <= 0) {
        LOGW("socket failed, try file");

        int fd = open(CONFIG_DIR "/native_bridge", O_RDONLY);
        if (fd == -1) {
            PLOGE("access " CONFIG_DIR "/native_bridge");
            return;
        }

        size = read(fd, buf, PATH_MAX);
        close(fd);

        if (size <= 0) {
            LOGE("can't read native_bridge");
            return;
        }
        buf[size] = 0;
        if (size > 1 && buf[size - 1] == '\n') buf[size - 1] = 0;
    } else {
        size = buf_size;
    }

    LOGI("original native bridge: %s", buf);

    if (buf[0] == '0' && buf[1] == 0) {
        return;
    }

    auto native_bridge = buf + size + 1;
    strcpy(native_bridge, LIB_PATH);
    strncat(native_bridge, buf, size);

    if (access(native_bridge, F_OK) != 0) {
        PLOGE("access %s", native_bridge);
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