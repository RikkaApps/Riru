#include <sys/stat.h>
#include <fcntl.h>
#include <climits>
#include <cstdio>
#include <sys/system_properties.h>
#include <unistd.h>
#include <cstdarg>
#include <sys/socket.h>
#include <sys/un.h>
#include <socket.h>
#include <sys/sendfile.h>
#include <cinttypes>
#include "status.h"
#include "logging.h"
#include "misc.h"
#include "module.h"
#include "config.h"
#include "daemon/status.h"

#define TMP_DIR "/dev"

static uint8_t WriteToSocket(uint8_t *buffer, uint32_t buffer_size) {
    struct sockaddr_un addr{};
    int fd;
    socklen_t socklen;
    uint8_t reply = Status::CODE_FAILED;

    if ((fd = socket(PF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0) {
        PLOGE("socket");
        goto clean;
    }

    socklen = setup_sockaddr(&addr, SOCKET_ADDRESS);

    if (connect(fd, (struct sockaddr *) &addr, socklen) == -1) {
        PLOGE("connect %s", SOCKET_ADDRESS);
        goto clean;
    }

    if (write_full(fd, &Status::ACTION_WRITE_STATUS, sizeof(Status::ACTION_WRITE_STATUS)) != 0
        || write_full(fd, &buffer_size, sizeof(buffer_size)) != 0
        || write_full(fd, buffer, buffer_size) != 0) {
        PLOGE("write %s", SOCKET_ADDRESS);
        goto clean;
    }

    if (read_full(fd, &reply, sizeof(reply))) {
        goto clean;
        PLOGE("read %s", SOCKET_ADDRESS);
    }

    LOGD("socket reply: %u", reply);

    clean:
    if (fd != -1) close(fd);
    return reply;
}

static void WriteToSocket(const flatbuffers::FlatBufferBuilder &builder) {
    LOGV("try write status via socket");
    if (WriteToSocket(builder.GetBufferPointer(), builder.GetSize()) == Status::CODE_OK) {
        LOGV("write to socket succeed");
    } else {
        LOGW("write to socket failed");
    }
}

void Status::WriteSelfAndModules() {
#ifdef __LP64__
    bool is64bit = true;
#else
    bool is64bit = false;
#endif

    flatbuffers::FlatBufferBuilder builder;

    auto core = CreateCoreDirect(
            builder,
            RIRU_API_VERSION,
            RIRU_VERSION_CODE,
            RIRU_VERSION_NAME,
            is_hide_enabled());

    std::vector<flatbuffers::Offset<Module>> modules_vector;
    for (auto module : *get_modules()) {
        if (strcmp(module->id, MODULE_NAME_CORE) == 0) continue;
        modules_vector.emplace_back(CreateModuleDirect(
                builder,
                module->id,
                module->apiVersion,
                module->version,
                module->versionName,
                module->supportHide));
    }

    auto modules = builder.CreateVector(modules_vector);

    FbStatusBuilder status_builder(builder);
    status_builder.add_is_64bit(is64bit);
    status_builder.add_core(core);
    status_builder.add_modules(modules);
    FinishFbStatusBuffer(builder, status_builder.Finish());

    WriteToSocket(builder);
}

void Status::WriteMethod(Method method, bool replaced, const char *sig) {
#ifdef __LP64__
    bool is64bit = true;
#else
    bool is64bit = false;
#endif

    static const char *method_name[Method::COUNT] = {
            "nativeForkAndSpecialize",
            "nativeForkSystemServer",
            "nativeSpecializeAppProcess"
    };

    flatbuffers::FlatBufferBuilder builder;

    flatbuffers::Offset<JNIMethod> method_data[] = {CreateJNIMethodDirect(builder, method_name[method], sig, replaced)};
    auto methods = builder.CreateVector(method_data, 1);

    FbStatusBuilder status_builder(builder);
    status_builder.add_is_64bit(is64bit);
    status_builder.add_jni_methods(methods);
    FinishFbStatusBuffer(builder, status_builder.Finish());

    WriteToSocket(builder);
}

static uint8_t ReadModulesFromSocket(uint8_t *&buffer, uint32_t &buffer_size) {
    struct sockaddr_un addr{};
    int fd;
    socklen_t socklen;
    uint8_t reply = Status::CODE_FAILED;
    flatbuffers::Verifier *verifier = nullptr;

    if ((fd = socket(PF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0) {
        PLOGE("socket");
        goto clean;
    }

    socklen = setup_sockaddr(&addr, SOCKET_ADDRESS);

    if (connect(fd, (struct sockaddr *) &addr, socklen) == -1) {
        PLOGE("connect %s", SOCKET_ADDRESS);
        goto clean;
    }

    if (write_full(fd, &Status::ACTION_READ_STATUS, sizeof(Status::ACTION_READ_STATUS)) != 0) {
        PLOGE("write %s", SOCKET_ADDRESS);
        goto clean;
    }

    if (read_full(fd, &reply, sizeof(reply)) != 0) {
        PLOGE("read %s", SOCKET_ADDRESS);
        goto clean;
    }

    if (read_full(fd, &buffer_size, sizeof(buffer_size)) != 0) {
        PLOGE("read %s", SOCKET_ADDRESS);
        goto clean;
    }

    buffer = (uint8_t *) malloc(buffer_size);
    if (read_full(fd, buffer, buffer_size) != 0) {
        PLOGE("read %s", SOCKET_ADDRESS);
        goto clean;
    }

    verifier = new flatbuffers::Verifier(buffer, (size_t) buffer_size);
    if (!Status::VerifyFbStatusBuffer(*verifier)) {
        LOGW("invalid data");
        goto clean;
    }

    LOGD("socket reply: %u", reply);

    clean:
    if (fd != -1) close(fd);
    delete verifier;
    return reply;
}

bool Status::ReadModules(uint8_t *&buffer, uint32_t &buffer_size) {
    LOGV("try read status via socket");
    if (ReadModulesFromSocket(buffer, buffer_size) == Status::CODE_OK) {
        LOGV("read from socket succeed");
        return true;
    } else {
        LOGW("read from socket failed");
        return false;
    }
}

bool Status::ReadFile(const char *path, int target_fd) {
    struct sockaddr_un addr{};
    uint32_t path_size = strlen(path);
    int32_t reply;
    int32_t file_size;
    int fd;
    socklen_t socklen;
    uint32_t buffer_size = 1024 * 8;
    char buffer[buffer_size];
    size_t bytes_size = 0;
    bool res = false;

    if ((fd = socket(PF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0) {
        PLOGE("socket");
        goto clean;
    }

    socklen = setup_sockaddr(&addr, SOCKET_ADDRESS);

    if (connect(fd, (struct sockaddr *) &addr, socklen) == -1) {
        PLOGE("connect %s", SOCKET_ADDRESS);
        goto clean;
    }

    if (write_full(fd, &ACTION_READ_FILE, sizeof(uint32_t)) != 0
        || write_full(fd, &path_size, sizeof(uint32_t)) != 0
        || write_full(fd, path, path_size) != 0) {
        PLOGE("write %s", SOCKET_ADDRESS);
        goto clean;
    }

    if (read_full(fd, &reply, sizeof(int32_t)) != 0) {
        PLOGE("read %s", SOCKET_ADDRESS);
        goto clean;
    }

    if (reply != 0) {
        LOGE("open %s failed with %d from remote: %s", path, reply, strerror(reply));
        errno = reply;
        goto clean;
    }

    if (read_full(fd, &file_size, sizeof(uint32_t)) != 0) {
        PLOGE("read %s", SOCKET_ADDRESS);
        goto clean;
    }

    LOGD("%s size %d", path, file_size);

    if (file_size > 0) {
        if (ftruncate(fd, file_size) == -1) {
            PLOGE("ftruncate");
            goto clean;
        }

        while (file_size > 0) {
            LOGD("attempt to read %d bytes", (int) buffer_size);
            auto read_size = TEMP_FAILURE_RETRY(read(fd, buffer, buffer_size));
            if (read_size == -1) {
                PLOGE("read");
                goto clean;
            }

            file_size -= read_size;
            bytes_size += read_size;
            LOGD("read %d bytes (total %d)", (int) read_size, (int) bytes_size);

            auto write_size = TEMP_FAILURE_RETRY(write(target_fd, buffer, read_size));
            if (write_size == -1) {
                PLOGE("read");
                goto clean;
            }
        }
        res = true;
    }

    clean:
    if (fd != -1) close(fd);
    return res;
}

void Status::ReadMagiskTmpfsPath(char *&buffer, int32_t &buffer_size) {
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

    if (write_full(fd, &ACTION_READ_MAGISK_TMPFS_PATH, sizeof(ACTION_READ_MAGISK_TMPFS_PATH)) != 0) {
        PLOGE("write %s", SOCKET_ADDRESS);
        goto clean;
    }

    if (read_full(fd, &buffer_size, sizeof(buffer_size)) != 0) {
        PLOGE("read %s", SOCKET_ADDRESS);
        goto clean;
    }

    LOGD("size=%d", buffer_size);

    if (buffer_size > 0 && buffer_size < PATH_MAX) {
        buffer = (char *) malloc(buffer_size + 1);
        memset(buffer, 0, buffer_size + 1);
        if (read_full(fd, buffer, buffer_size) != 0) {
            PLOGE("read %s", SOCKET_ADDRESS);
            goto clean;
        }
    }

    clean:
    if (fd != -1) close(fd);
}