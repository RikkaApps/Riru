#define LOG_TAG    "rirud"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <logging.h>
#include <config.h>
#include <socket.h>
#include <misc.h>
#include <flatbuffers/flatbuffers.h>
#include <selinux.h>
#include <cinttypes>
#include <wait.h>
#include <dirent.h>
#include <sys/system_properties.h>
#include "status.h"
#include "status_generated.h"
#include "setproctitle.h"

#define WORKER_PROCESS "rirud_worker"

static char original_native_bridge[PROP_VALUE_MAX] = {'0', '\0'};

static int server_socket_fd = -1;
static std::vector<pid_t> child_pids;

static bool handle_ping(int sockfd) {
    return write_full(sockfd, &Status::CODE_OK, sizeof(Status::CODE_OK)) == 0;
}

static bool handle_read_status(int sockfd) {
    flatbuffers::FlatBufferBuilder builder;
    Status::ReadFromFile(builder);

    auto buf = builder.GetBufferPointer();
    auto size = (uint32_t) builder.GetSize();

    return write_full(sockfd, &Status::CODE_OK, sizeof(Status::CODE_OK)) == 0
           && write_full(sockfd, &size, sizeof(size)) == 0
           && write_full(sockfd, buf, size) == 0;
}

static bool handle_read_original_native_bridge(int sockfd) {
    int32_t size = strlen(original_native_bridge);
    return write_full(sockfd, &size, sizeof(size)) == 0 && (size <= 0 || write_full(sockfd, original_native_bridge, size) == 0);
}

static bool handle_read_magisk_tmpfs_path(int sockfd) {
    auto path = Status::GetMagiskTmpfsPath();
    int32_t size = strlen(path);
    return write_full(sockfd, &size, sizeof(size)) == 0 && (size <= 0 || write_full(sockfd, path, size) == 0);
}

static bool handle_write_status(int sockfd) {
    uint8_t *buf;
    uint32_t size;

    if (read_full(sockfd, &size, sizeof(size)) == -1) {
        PLOGE("read");
        return false;
    }

    buf = (uint8_t *) malloc(size);
    if (read_full(sockfd, buf, size) == -1) {
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
    write_full(sockfd, &Status::CODE_OK, sizeof(Status::CODE_OK));
    free(buf);
    return true;
}

static bool handle_read_file(int sockfd) {
    char path[PATH_MAX]{0};
    uint32_t size;
    int32_t reply;
    ssize_t res;
    int32_t file_size;
    struct stat st{};

    if (read_full(sockfd, &size, sizeof(uint32_t)) == -1
        || read_full(sockfd, path, size) == -1) {
        PLOGE("read");
        return false;
    }

    LOGI("read file %s", path);

    errno = 0;
    int fd = open(path, O_RDONLY);
    reply = (int32_t) errno;

    if (fd == -1) {
        PLOGE("open %s", path);
    }

    if (write_full(sockfd, &reply, sizeof(int32_t)) == -1) {
        PLOGE("write errno");
        return false;
    }

    if (fd == -1) {
        return true;
    }

    if (fstat(fd, &st) == 0) {
        file_size = (int32_t) st.st_size;
    } else {
        file_size = -1;
    }

    if (write_full(sockfd, &file_size, sizeof(int32_t)) == -1) {
        PLOGE("write bytes count");
        return false;
    }

    if (file_size > 0) {
        auto bytes_remaining = (size_t) file_size;
        size_t count;
        do {
            count = bytes_remaining > 0x7ffff000 ? 0x7ffff000 : (size_t) bytes_remaining;
            LOGV("attempt to send %" PRIuPTR" bytes", count);

            res = sendfile(sockfd, fd, nullptr, count);
            if (res == -1) {
                PLOGE("sendfile");
            } else {
                LOGV("sent %" PRIdPTR " bytes", res);
                bytes_remaining -= res;
            }
        } while (bytes_remaining > 0);
    } else if (file_size == -1) {
        LOGW("%s don't has size, fallback to read and write", path);

        size_t buffer_size = 8192;
        char buffer[buffer_size];
        while ((res = TEMP_FAILURE_RETRY(read(fd, buffer, buffer_size))) > 0) {
            LOGV("sent %" PRIdPTR " bytes", res);
            TEMP_FAILURE_RETRY(write(sockfd, buffer, res));
        }
    } else {
        LOGV("%s is a empty file", path);
    }
    close(fd);

    return true;
}

static bool handle_read_dir(int sockfd) {
    char path[PATH_MAX]{0};
    uint32_t size;
    int32_t reply;
    uint8_t continue_read;
    DIR *dir = nullptr;
    dirent *dirent = nullptr;
    bool res = true;

    if (read_full(sockfd, &size, sizeof(uint32_t)) == -1
        || read_full(sockfd, path, size) == -1) {
        PLOGE("read");
        goto failed;
    }

    LOGI("read dir %s", path);

    errno = 0;
    dir = opendir(path);
    reply = (int32_t) errno;

    if (dir == nullptr) {
        PLOGE("opendir %s", path);
    }

    if (write_full(sockfd, &reply, sizeof(int32_t)) == -1) {
        PLOGE("write errno");
        goto failed;
    }

    if (dir == nullptr) {
        goto clean;
    }

    for (;;) {
        if (read_full(sockfd, &continue_read, sizeof(uint8_t)) == -1) {
            PLOGE("read");
            goto failed;
        }

        if (continue_read == 0) {
            goto clean;
        }

        errno = 0;
        dirent = readdir(dir);
        reply = (int32_t) errno;

        if (dirent == nullptr && reply == 0) {
            reply = -1;
        }

        if (write_full(sockfd, &reply, sizeof(int32_t)) == -1) {
            PLOGE("write");
            goto failed;
        }

        if (dirent == nullptr) {
            if (reply == -1) {
                goto clean;
            } else {
                continue;
            }
        }

        if (write_full(sockfd, &dirent->d_type, sizeof(unsigned char)) == -1
            || write_full(sockfd, &dirent->d_name, 256) == -1) {
            PLOGE("write");
            goto failed;
        }
    }

    failed:
    res = false;
    clean:
    if (dir) closedir(dir);
    return res;
}

static void handle_socket(int sockfd, uint32_t action) {
    switch (action) {
        case Status::ACTION_PING: {
            LOGI("action: ping");
            handle_ping(sockfd);
            break;
        }
        case Status::ACTION_READ_STATUS: {
            LOGI("action: read status");
            handle_read_status(sockfd);
            break;
        }
        case Status::ACTION_READ_NATIVE_BRIDGE: {
            LOGI("action: read original native bridge");
            handle_read_original_native_bridge(sockfd);
            break;
        }
        case Status::ACTION_READ_MAGISK_TMPFS_PATH: {
            LOGI("action: read Magisk tmpfs path");
            handle_read_magisk_tmpfs_path(sockfd);
            break;
        }
        case Status::ACTION_WRITE_STATUS: {
            LOGI("action: write status");
            handle_write_status(sockfd);
            break;
        }
        case Status::ACTION_READ_FILE: {
            LOGI("action: read file");
            handle_read_file(sockfd);
            break;
        }
        case Status::ACTION_READ_DIR: {
            LOGI("action: read dir");
            handle_read_dir(sockfd);
            break;
        }
        default:
            break;
    }
}

static void handle_socket(int sockfd) {
    uint32_t action;
    bool first = true;

    while (true) {
        if (read_full(sockfd, &action, sizeof(action)) == -1) {
            if (first) {
                PLOGE("read action");
            } else {
                LOGI("no next action, exiting...");
            }
            return;
        }

        handle_socket(sockfd, action);
        first = false;
    }
}

static void socket_server() {
    int clifd;
    struct sockaddr_un addr{};
    struct sockaddr_un from{};
    socklen_t fromlen = sizeof(from);
    struct ucred cred{};
    pid_t pid;

    if (setsockcreatecon("u:r:zygote:s0") != 0) {
        PLOGE("setsockcreatecon");
    }

    if ((server_socket_fd = socket(PF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0) {
        PLOGE("socket");
        return;
    }

    socklen_t socklen = setup_sockaddr(&addr, SOCKET_ADDRESS);
    if (bind(server_socket_fd, (sockaddr *) (&addr), socklen) < 0) {
        PLOGE("bind %s", SOCKET_ADDRESS);
        return;
    }
    LOGI("socket " SOCKET_ADDRESS" created");

    if (listen(server_socket_fd, 10) == -1) {
        PLOGE("listen");
        return;
    }

    while (true) {
        clifd = accept4(server_socket_fd, (struct sockaddr *) &from, &fromlen, SOCK_CLOEXEC);
        if (clifd == -1) {
            if (errno == EINTR) {
                if (server_socket_fd == -1) {
                    LOGI("interrupted system call");
                    return;
                } else {
                    continue;
                }
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

        pid = fork();
        if (pid == 0) {
            setproctitle(WORKER_PROCESS);
            handle_socket(clifd);
            close(clifd);
            exit(0);
        } else {
            LOGI("forked process %d", pid);
            if (pid != -1) {
                child_pids.emplace_back(pid);
            }
        }

        clean:
        close(clifd);
    }
}

static void sig_handler(int sig) {
    LOGD("sig %d", sig);

    if (sig == SIGUSR1) {
        if (server_socket_fd != -1) {
            LOGI("close socket");
            close(server_socket_fd);
            server_socket_fd = -1;
        } else {
            LOGW("socket is not running?");
        }
    } else if (sig == SIGCHLD) {
        int status;
        pid_t pid;
        auto it = child_pids.begin();
        while (it != child_pids.end()) {
            pid = *it;
            if (pid == waitpid(pid, &status, WNOHANG)) {
                if (WIFEXITED(status)) {
                    int returned = WEXITSTATUS(status);
                    LOGD("%d exited normally with status %d", pid, returned);
                } else if (WIFSIGNALED(status)) {
                    int signum = WTERMSIG(status);
                    LOGD("%d exited due to receiving signal %d", pid, signum);
                } else if (WIFSTOPPED(status)) {
                    int signum = WSTOPSIG(status);
                    LOGD("%d stopped due to receiving signal %d", pid, signum);
                } else {
                    LOGD("%d something strange just happened", pid);
                }

                it = child_pids.erase(it);
            } else ++it;
        }
    }
}

[[noreturn]] static void daemon_main() {
    int sig;
    sigset_t s;
    struct sigaction act{}, act2{};

    setproctitle("rirud");

    Status::GenerateRandomName();

    act.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &act, nullptr);

    act2.sa_handler = sig_handler;
    sigaction(SIGUSR1, &act2, nullptr);

    signal(SIGCHLD, sig_handler);

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
    if (__system_property_get("ro.dalvik.vm.native.bridge", original_native_bridge) > 0) {
        LOGI("backup original native bridge %s", original_native_bridge);
    } else {
        PLOGE("getprop ro.dalvik.vm.native.bridge");
    }

    LOGI("Magisk version is %d", Status::GetMagiskVersion());
    LOGI("Magisk tmpfs path is %s", Status::GetMagiskTmpfsPath());

    switch (daemon(0, 0)) {
        case -1:
            PLOGE("daemon");
            return -1;
        default:
            daemon_main();
    }
}