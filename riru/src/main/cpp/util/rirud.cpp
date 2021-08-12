#include <riru.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <logging.h>
#include <config.h>
#include <socket.h>
#include <unistd.h>
#include <malloc.h>
#include <vector>
#include <string>
#include <dirent.h>
#include "rirud.h"
#include "module.h"
#include "finally.h"

bool RirudSocket::Write(std::string_view str) const {
    auto count = str.size();
    const auto *buf = str.data();
    return Write<uint32_t>(str.size()) && Write(buf, count);
}

bool RirudSocket::Read(std::string &str) const {
    uint32_t size;
    if (!Read(size) || size < 0) return false;
    str.resize(size);
    return Read(str.data(), size);
}


RirudSocket::RirudSocket(unsigned retries) {
    if ((fd_ = socket(PF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0) {
        return;
    }

    struct sockaddr_un addr{
            .sun_family = AF_UNIX,
            .sun_path={0}
    };
    strncpy(addr.sun_path + 1, RIRUD.data(), RIRUD.size());
    socklen_t socklen = sizeof(sa_family_t) + strlen(addr.sun_path + 1) + 1;

    while (retries-- > 0) {
        if (connect(fd_, reinterpret_cast<struct sockaddr *>(&addr), socklen) != -1) return;
        LOGW("retrying to connect rirud in 1s");
        sleep(1);
    }
    close(fd_);
    fd_ = -1;
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

std::string RirudSocket::ReadMagiskTmpfsPath() const {
    std::string result;
    if (Write(Action::READ_MAGISK_TMPFS_PATH)) {
        Read(result);
    }
    return result;
}

std::string RirudSocket::ReadNativeBridge() const {
    std::string result;
    if (Write(Action::READ_NATIVE_BRIDGE)) {
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

bool RirudSocket::Write(const void *buf, size_t len) const {
    auto count = len;
    while (count > 0) {
        ssize_t size = write(fd_, buf, count < SSIZE_MAX ? count : SSIZE_MAX);
        if (size == -1) {
            if (errno == EINTR) continue;
            else return false;
        }
        buf = static_cast<const char *>(buf) + size;
        count -= size;
    }
    return true;
}

bool RirudSocket::Read(void *out, size_t len) const {
    while (len > 0) {
        ssize_t ret = read(fd_, out, len);
        if (ret <= 0) {
            if (errno == EINTR) continue;
            else return false;
        }
        out = static_cast<char *>(out) + ret;
        len -= ret;
    }
    return true;
}
