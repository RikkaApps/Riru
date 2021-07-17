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
#include "module.h"
#include "finally.h"

static int socket_fd = -1;

namespace {
    template<typename T>
    inline std::enable_if_t<std::is_fundamental_v<T> || std::is_enum_v<T>, int>
    WriteFull(int fd, const T &data) {
        return write_full(fd, &data, sizeof(T));
    }

    int WriteFull(int fd, std::string_view data) {
        return write_full(fd, data.data(), data.size());
    }

}  // namespace

bool RirudSocket::Write(std::string_view str) const {
    auto count = str.size();
    const auto *buf = str.data();
    return Write<uint32_t>(str.size()) && Write(buf, count);
}

bool RirudSocket::Read(std::string &str) const {
    uint32_t size;
    if (!Read(size)) return false;
    str.resize(size);
    return Read(str.data(), size);
}


RirudSocket::RirudSocket() {
    if ((fd_ = socket(PF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0) {
        return;
    }

    struct sockaddr_un addr{
            .sun_family = AF_UNIX,
            .sun_path={0}
    };
    strcpy(addr.sun_path + 1, "rirud");
    socklen_t socklen = sizeof(sa_family_t) + strlen(addr.sun_path + 1) + 1;

    if (connect(fd_, reinterpret_cast<struct sockaddr *>(&addr), socklen) == -1) {
        close(fd_);
        fd_ = -1;
        return;
    }
}

RirudSocket::~RirudSocket() {
    if (fd_ != -1) {
        close(fd_);
    }
}

void RirudSocket::DirIter::ContinueRead() {
    int32_t reply;
    unsigned char type;
    if (!socket_.Write(continue_read) || !socket_.Read(reply) || reply != 0 ||
        !socket_.Read(type) || !socket_.Read(path.data(), MAX_PATH_SIZE)) {
        path[0] = '\0';
    }
}

std::string RirudSocket::ReadMagiskTmpfsPath() {
    int32_t size;
    std::string result;
    if (Write(Action::READ_MAGISK_TMPFS_PATH) && Read(size) && size > 0) {
        result.resize(size);
        Read(result);
    }
    return result;
}

std::string RirudSocket::ReadNativeBridge() {
    int32_t size;
    std::string result;
    if (Write(Action::READ_NATIVE_BRIDGE) && Read(size) && size > 0) {
        result.resize(size);
        Read(result);
    }
    return result;
}

std::string RirudSocket::ReadFile(std::string_view path) {
    Write(Action::READ_FILE);
    Write(path);
    int32_t rirud_errno;
    Read(rirud_errno);
    if (rirud_errno != 0) {
        return {};
    }
    std::string content;
    Read(content);
    return content;
}

bool RirudSocket::Write(const char *buf, size_t len) const {
    auto count = len;
    while (count > 0) {
        ssize_t size = write(fd_, buf, count < SSIZE_MAX ? count : SSIZE_MAX);
        if (size == -1) {
            if (errno == EINTR) continue;
            else return false;
        }
        buf = buf + size;
        count -= size;
    }
    return true;
}

bool RirudSocket::Read(char *out, size_t len) const {
    while (len > 0) {
        ssize_t ret = read(fd_, out, len);
        if (ret <= 0) {
            if (errno == EINTR) continue;
            else return false;
        }
        out = out + ret;
        len -= ret;
    }
    return true;
}
