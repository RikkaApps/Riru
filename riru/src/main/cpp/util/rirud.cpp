#include <riru.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <logging.h>
#include <config.h>
#include <socket.h>
#include <unistd.h>
#include <misc.h>
#include <malloc.h>
#include <vector>
#include <string>
#include <dirent.h>
#include "rirud.h"

static int socket_fd = -1;

int rirud::OpenSocket() {
    if (socket_fd != -1) return socket_fd;

    int fd;
    struct sockaddr_un addr{};
    socklen_t socklen;

    if ((fd = socket(PF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0) {
        PLOGE("socket");
        return -1;
    }

    socklen = setup_sockaddr(&addr, SOCKET_ADDRESS);

    if (connect(fd, (struct sockaddr *) &addr, socklen) == -1) {
        PLOGE("connect %s", SOCKET_ADDRESS);
        close(fd);
        return -1;
    }

    socket_fd = fd;
    return fd;
}

void rirud::CloseSocket() {
    if (socket_fd != -1) {
        close(socket_fd);
        socket_fd = -1;
    }
}

ssize_t rirud::ReadMagiskTmpfsPath(char *buffer) {
    int fd = OpenSocket();
    if (fd == -1) {
        LOGD("read magisk path: cannot open socket");
        return -1;
    }

    if (write_full(fd, &ACTION_READ_MAGISK_TMPFS_PATH, sizeof(ACTION_READ_MAGISK_TMPFS_PATH)) != 0) {
        PLOGE("write %s", SOCKET_ADDRESS);
        return -1;
    }

    int32_t buffer_size = -1;
    if (read_full(fd, &buffer_size, sizeof(buffer_size)) != 0) {
        PLOGE("read %s", SOCKET_ADDRESS);
        return -1;
    }

    LOGD("size=%d", buffer_size);

    if (buffer_size > 0 && buffer_size < PATH_MAX) {
        memset(buffer, 0, buffer_size + 1);
        if (read_full(fd, buffer, buffer_size) != 0) {
            PLOGE("read %s", SOCKET_ADDRESS);
            return -1;
        }
    }

    return buffer_size;
}

void rirud::WriteModules(const std::vector<Module> &modules) {
    int fd = OpenSocket();
    if (fd == -1) {
        LOGD("write modules: cannot open socket");
        return;
    }

#ifdef __LP64__
    uint8_t is64bit = 1;
#else
    uint8_t is64bit = 0;
#endif
    uint32_t count = modules.size();

    if (write_full(fd, &rirud::ACTION_WRITE_STATUS, sizeof(rirud::ACTION_WRITE_STATUS)) != 0
        || write_full(fd, &is64bit, sizeof(is64bit)) != 0
        || write_full(fd, &count, sizeof(count)) != 0) {
        PLOGE("write %s", SOCKET_ADDRESS);
        return;
    }

    for (const auto &module : modules) {
        uint32_t id_len = strlen(module.id);
        write_full(fd, &id_len, sizeof(id_len));
        write_full(fd, module.id, id_len);

        write_full(fd, &module.apiVersion, sizeof(module.apiVersion));
        write_full(fd, &module.version, sizeof(module.version));

        uint32_t version_name_len = strlen(module.versionName);
        write_full(fd, &version_name_len, sizeof(version_name_len));
        write_full(fd, module.versionName, version_name_len);

        write_full(fd, &module.supportHide, sizeof(module.supportHide));
    }
}

bool rirud::ReadDir(const char *path, std::vector<std::string> &dirs) {
    int fd = OpenSocket();
    if (fd == -1) {
        LOGD("read legacy modules: cannot open socket");
        return false;
    }

    uint32_t path_size = strlen(path);
    int32_t reply;
    bool res = false;
    bool continue_read = true;
    dirent dirent{};

    if (write_full(fd, &rirud::ACTION_READ_DIR, sizeof(uint32_t)) != 0
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
        LOGE("opendir %s failed with %d from remote: %s", path, reply, strerror(reply));
        errno = reply;
        goto clean;
    }

    while (true) {
        if (write_full(fd, &continue_read, sizeof(uint8_t)) != 0) {
            PLOGE("write %s", SOCKET_ADDRESS);
            goto clean;
        }

        if (read_full(fd, &reply, sizeof(int32_t)) != 0) {
            PLOGE("read %s", SOCKET_ADDRESS);
            goto clean;
        }

        if (reply == -1) {
            res = true;
            goto clean;
        }

        if (reply != 0) {
            LOGE("opendir %s failed with %d from remote: %s", path, reply, strerror(reply));
            continue;
        }

        if (read_full(fd, &dirent.d_type, sizeof(unsigned char)) != 0
            || read_full(fd, dirent.d_name, 256) != 0) {
            PLOGE("read %s", SOCKET_ADDRESS);
            goto clean;
        }

        if (dirent.d_name[0] != '.') {
            dirs.emplace_back(dirent.d_name);
        }
    }

    clean:
    return res;
}
