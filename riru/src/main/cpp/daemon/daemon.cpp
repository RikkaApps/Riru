#define LOG_TAG    "rirud"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <logging.h>
#include <config.h>
#include <socket.h>
#include <misc.h>
#include <flatbuffers/flatbuffers.h>
#include <selinux.h>
#include "status.h"
#include "status_generated.h"

static int socket_fd = -1;

static bool handle_ping(int fd) {
    return write_full(fd, &Status::CODE_OK, sizeof(Status::CODE_OK)) == 0;
}

static bool handle_read(int fd) {
    flatbuffers::FlatBufferBuilder builder;
    Status::ReadFromFile(builder);

    auto buf = builder.GetBufferPointer();
    auto size = (uint32_t) builder.GetSize();

    return write_full(fd, &Status::CODE_OK, sizeof(Status::CODE_OK)) == 0
           && write_full(fd, &size, sizeof(size)) == 0
           && write_full(fd, buf, size) == 0;
}

static bool handle_read_original_native_bridge(int clifd) {
    char buf[PATH_MAX]{0};
    int32_t size = 0;
    int fd = open(CONFIG_DIR "/native_bridge", O_RDONLY);
    if (fd == -1) {
        PLOGE("access " CONFIG_DIR "/native_bridge");
    } else {
        size = read(fd, buf, PATH_MAX);
        close(fd);
    }

    return write_full(clifd, &size, sizeof(size)) == 0 && (size <= 0 || write_full(clifd, buf, size) == 0);
}

static bool handle_write(int fd) {
    uint8_t *buf;
    uint32_t size;

    if (read_full(fd, &size, sizeof(size)) == -1) {
        PLOGE("read");
        return false;
    }

    buf = (uint8_t *) malloc(size);
    if (read_full(fd, buf, size) == -1) {
        PLOGE("read");
        free(buf);
        return false;
    }

    flatbuffers::Verifier verifier = flatbuffers::Verifier(buf, (size_t) size);
    if (!Status::VerifyFbStatusBuffer(verifier)) {
        LOGW("invalid data");
        free(buf);
        return false;
    }

    Status::WriteToFile(Status::GetFbStatus(buf));
    write_full(fd, &Status::CODE_OK, sizeof(Status::CODE_OK));
    free(buf);
    return true;
}

static void socket_server() {
    int clifd;
    struct sockaddr_un addr{};
    struct sockaddr_un from{};
    uint32_t action;
    socklen_t fromlen;
    struct ucred cred{};

    if (setsockcreatecon("u:r:zygote:s0") != 0) {
        PLOGE("setsockcreatecon");
    }

    if ((socket_fd = socket(PF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0) {
        PLOGE("socket");
        return;
    }

    socklen_t socklen = setup_sockaddr(&addr, SOCKET_ADDRESS);
    if (bind(socket_fd, (sockaddr *) (&addr), socklen) < 0) {
        PLOGE("bind %s", SOCKET_ADDRESS);
        return;
    }
    LOGI("socket " SOCKET_ADDRESS" created");

    listen(socket_fd, 10);

    while (true) {
        clifd = accept4(socket_fd, (struct sockaddr *) &from, &fromlen, SOCK_CLOEXEC);
        if (clifd == -1) {
            if (errno == EINTR) {
                LOGI("interrupted system call");
                return;
            } else {
                PLOGE("accept");
                continue;
            }
        }

        if (get_client_cred(clifd, &cred) == 0) {
            if (cred.uid != 0) {
                LOGE("accept not from root (uid=%d, pid=%d)", cred.uid, cred.pid);
                goto clean;
            }
        }

        if (read_full(clifd, &action, sizeof(action)) == -1) {
            PLOGE("read");
            goto clean;
        }

        switch (action) {
            case Status::ACTION_PING: {
                LOGI("socket request: ping");
                handle_ping(clifd);
                break;
            }
            case Status::ACTION_READ: {
                LOGI("socket request: read status");
                handle_read(clifd);
                break;
            }
            case Status::ACTION_READ_NATIVE_BRIDGE: {
                LOGI("socket request: read orignal native bridge");
                handle_read_original_native_bridge(clifd);
                break;
            }
            case Status::ACTION_WRITE: {
                LOGI("socket request: write status");
                handle_write(clifd);
                break;
            }
            default:
                break;
        }

        clean:
        close(clifd);
    }
}

static void sig_handler(int sig) {
    LOGD("sig %d", sig);

    if (sig == SIGUSR1) {
        if (socket_fd != -1) {
            LOGI("close socket");
            close(socket_fd);
        } else {
            LOGW("socket is not running?");
        }
    }
}

[[noreturn]] static void daemon_main() {
    int sig;
    sigset_t s;
    struct sigaction act{}, act2{};

    Status::GenerateRandomName();

    act.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &act, nullptr);

    act2.sa_handler = sig_handler;
    sigaction(SIGUSR1, &act2, nullptr);

    sigemptyset(&s);
    sigaddset(&s, SIGUSR2);
    sigprocmask(SIG_BLOCK, &s, nullptr);

    dload_selinux();

    while (true) {
        socket_server();
        sigwait(&s, &sig);
        LOGI("restart socket");
    }
}

int main(int argc, char **argv) {
    daemon(0, 0);

    switch (fork()) {
        case 0:
            daemon_main();
        case -1:
            PLOGE("fork");
            return -1;
        default:
            break;
    }
    return 0;
}