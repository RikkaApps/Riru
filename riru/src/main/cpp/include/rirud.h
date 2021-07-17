#ifndef RIRUD_H
#define RIRUD_H

#include <cstdint>
#include <vector>
#include <string>

#include <string>
#include "buff_string.h"

class RirudSocket {
public:

    enum class Action : uint32_t {
        READ_FILE = 4,
        READ_DIR = 5,

        // used by riru itself only, could be removed in the future
        WRITE_STATUS = 2,
        READ_NATIVE_BRIDGE = 3,
        READ_MAGISK_TMPFS_PATH = 6,
    };

    enum class CODE : uint8_t {
        OK = 0,
        FAILED = 1,
    };

    class DirIter {
        constexpr static uint8_t continue_read = true;
        constexpr static size_t MAX_PATH_SIZE = 256u;
    private:
        DirIter(std::string_view path, const RirudSocket &socket) : socket_(socket) {
            int32_t reply;
            if (socket_.Write(Action::READ_DIR) && socket_.Write(path) && socket_.Read(reply)) {
                ContinueRead();
            }
        }

        void ContinueRead();

        DirIter(const DirIter &) = delete;

        DirIter operator=(const DirIter &) = delete;


        const RirudSocket &socket_;
        std::array<char, MAX_PATH_SIZE> path;

        friend class RirudSocket;

    public:
        operator bool() {
            return path[0];
        }

        DirIter &operator++() {
            ContinueRead();
            return *this;
        }

        std::string_view operator*() {
            return {path.data()};
        }
    };

    friend class RirudSocket::DirIter;

    bool valid() const {
        return fd_ != -1;
    }

    RirudSocket();

    std::string ReadFile(std::string_view path);

    std::string ReadMagiskTmpfsPath();

    std::string ReadNativeBridge();

    DirIter ReadDir(std::string_view path) const {
        return {path, *this};
    }

    template<typename T>
    std::enable_if_t<std::is_fundamental_v<T> || std::is_enum_v<T>, bool>
    Read(T &obj) const {
        return Read(reinterpret_cast<char *>(&obj), sizeof(T));
    }

    bool Read(std::string &str) const;

    template<typename T>
    std::enable_if_t<std::is_fundamental_v<T> || std::is_enum_v<T>, bool>
    Write(const T &obj) const {
        return Write(reinterpret_cast<const char *>(&obj), sizeof(T));
    }

    bool Write(std::string_view str) const;

    ~RirudSocket();

private:
    RirudSocket(const RirudSocket &) = delete;

    RirudSocket operator=(const RirudSocket &) = delete;

    bool Write(const char *buf, size_t len) const;

    bool Read(char *buf, size_t len) const;

    int fd_ = -1;
};

#endif //RIRUD_H
