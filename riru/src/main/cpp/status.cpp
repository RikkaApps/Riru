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

static void TryWriteSocketOrFile(const flatbuffers::FlatBufferBuilder &builder) {
    LOGV("try write status via socket");
    if (WriteToSocket(builder.GetBufferPointer(), builder.GetSize()) == Status::CODE_OK) {
        LOGV("write to socket succeed");
    } else {
        LOGW("write to socket failed, try file");
        Status::WriteToFile(Status::GetFbStatus(builder.GetBufferPointer()));
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
        if (strcmp(module->name, MODULE_NAME_CORE) == 0) continue;
        modules_vector.emplace_back(CreateModuleDirect(
                builder,
                module->name,
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

    TryWriteSocketOrFile(builder);
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

    TryWriteSocketOrFile(builder);
}

static uint8_t ReadFromSocket(uint8_t *&buffer, uint32_t &buffer_size) {
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
        return reply;
    }

    LOGD("socket reply: %u", reply);

    clean:
    if (fd != -1) close(fd);
    delete verifier;
    return reply;
}

static void TryReadSocketOrFile(uint8_t *&buffer, uint32_t &buffer_size) {
    LOGV("try read status via socket");
    if (ReadFromSocket(buffer, buffer_size) == Status::CODE_OK) {
        LOGV("read from socket succeed");
    } else {
        LOGW("read from socket failed, try file");

        flatbuffers::FlatBufferBuilder builder;
        Status::ReadFromFile(builder);

        buffer_size = builder.GetSize();
        buffer = (uint8_t *) malloc(buffer_size);
        memcpy(buffer, builder.GetBufferPointer(), buffer_size);
    }
}

void Status::Read(uint8_t *&buffer, uint32_t &buffer_size) {
    TryReadSocketOrFile(buffer, buffer_size);
}