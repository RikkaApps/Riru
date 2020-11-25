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
#include "status.h"
#include "status_generated.h"

static bool handle_read(int fd) {
    flatbuffers::FlatBufferBuilder builder;
    Status::ReadFromFile(builder);

    auto buf = builder.GetBufferPointer();
    auto size = (uint32_t) builder.GetSize();

    return write_full(fd, &Status::CODE_OK, sizeof(Status::CODE_OK)) == 0
           && write_full(fd, &size, sizeof(size)) == 0
           && write_full(fd, buf, size) == 0;
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

[[noreturn]] static void socket_server() {
    int fd, clifd;
    struct sockaddr_un addr{};
    struct sockaddr_un from{};
    uint32_t action;
    socklen_t fromlen;
    struct ucred cred{};

    while (true) {
        if ((fd = socket(PF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0) {
            PLOGE("socket");
            sleep(1);
            continue;
        }

        socklen_t socklen = setup_sockaddr(&addr, SOCKET_ADDRESS);
        if (bind(fd, (sockaddr *) (&addr), socklen) < 0) {
            PLOGE("bind %s", SOCKET_ADDRESS);
            sleep(1);
            continue;
        }
        LOGI("socket " SOCKET_ADDRESS" created");

        listen(fd, 10);

        while (true) {
            clifd = accept4(fd, (struct sockaddr *) &from, &fromlen, SOCK_CLOEXEC);
            if (clifd == -1) {
                PLOGE("accept");
                break;
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
                case Status::ACTION_READ: {
                    LOGI("read status request from socket");
                    handle_read(clifd);
                    break;
                }
                case Status::ACTION_WRITE: {
                    LOGI("write file request from socket");
                    handle_write(clifd);
                    break;
                }
                default:
                    break;
            }

            clean:
            close(clifd);
        }

        LOGI("close");
        close(fd);
    }
}

static void start_socket_server() {
    // Ignore SIGPIPE
    struct sigaction act{};
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &act, nullptr);

    socket_server();
}

int main(int argc, char **argv) {
    daemon(0, 0);

    switch (fork()) {
        case 0:
            start_socket_server();
            break;
        case -1:
            PLOGE("fork");
            return -1;
        default:
            break;
    }
    return 0;
}