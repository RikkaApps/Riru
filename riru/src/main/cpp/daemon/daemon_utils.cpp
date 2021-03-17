#include <sys/stat.h>
#include <fcntl.h>
#include <climits>
#include <cstdio>
#include <sys/system_properties.h>
#include <unistd.h>
#include <cstdarg>
#include <malloc.h>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <vector>
#include <rirud.h>
#include "daemon_utils.h"
#include "logging.h"
#include "misc.h"
#include "config.h"

#define TMP_DIR "/dev"
#define DEV_RANDOM CONFIG_DIR "/dev_random"

static const char *GetRandomName() {
    static char *name = nullptr;
    if (name != nullptr) return name;

    size_t size = 7;
    auto tmp = static_cast<char *>(malloc(size + 1));
    const char *file = DEV_RANDOM;
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    int fd = open(file, O_RDONLY);
    if (fd != -1) {
        if (0 == read_full(fd, tmp, size)) {
            tmp[size] = '\0';
            close(fd);
            name = tmp;
            return name;
        }
        close(fd);

        LOGE("bad %s", DEV_RANDOM);
    }

    srand(time(nullptr));
    for (size_t n = 0; n < size; n++) {
        auto key = rand() % (sizeof charset - 1);
        tmp[n] = charset[key];
    }
    tmp[size] = '\0';

    mkdir("/data/adb/riru", 0700);
    fd = open(file, O_CREAT | O_WRONLY | O_TRUNC, 0700);
    if (fd == -1) {
        PLOGE("open %s", file);
        return nullptr;
    }
    write_full(fd, tmp, size);
    close(fd);

    name = tmp;
    return name;
}

void Daemon::GenerateRandomName() {
    GetRandomName();
}

static int OpenFile(bool is_64bit, const char *name, ...) {
    auto random_name = GetRandomName();
    if (random_name == nullptr) {
        LOGE("unable to get random name");
        return -1;
    }
    LOGD("random name is %s", random_name);

    const char *filename, *next;
    char dir[PATH_MAX];

    strcpy(dir, TMP_DIR);
    if (is_64bit) {
        strcat(dir, "/riru64_");
    } else {
        strcat(dir, "/riru_");
    }
    strcat(dir, random_name);


    va_list va;
    va_start(va, name);
    while (true) {
        next = va_arg(va, const char *);
        if (next == nullptr) {
            filename = name;
            break;
        }
        strcat(dir, "/");
        strcat(dir, name);
        name = next;
    }
    va_end(va);

    LOGD("open %s/%s", dir, filename);

    int dir_fd = open(dir, O_DIRECTORY);
    if (dir_fd == -1 && mkdirs(dir, 0700) == 0) {
        dir_fd = open(dir, O_DIRECTORY);
    }
    if (dir_fd == -1) {
        PLOGE("cannot open dir %s", dir);
        return -1;
    }

    int fd = openat(dir_fd, filename, O_CREAT | O_WRONLY | O_TRUNC, 0700);
    if (fd < 0) {
        PLOGE("unable to create/open %s", name);
    }

    close(dir_fd);
    return fd;
}

#define OpenFile(...) OpenFile(is_64bit, __VA_ARGS__, nullptr)

void Daemon::WriteToFile(bool is_64bit, uint32_t count, const rirud::Module* modules) {
    char buf[1024];
    int fd;

    if ((fd = OpenFile("api")) != -1) {
        write_full(fd, buf, sprintf(buf, "%d", RIRU_API_VERSION));
        close(fd);
    }

    if ((fd = OpenFile("version")) != -1) {
        write_full(fd, buf, sprintf(buf, "%d", RIRU_VERSION_CODE));
        close(fd);
    }

    if ((fd = OpenFile("version_name")) != -1) {
        write_full(fd, buf, sprintf(buf, "%s", RIRU_VERSION_NAME));
        close(fd);
    }

    for (int i = 0; i < count; ++i) {
        auto module = modules[i];
        auto name = module.id;

        if ((fd = OpenFile("modules", name, "hide")) != -1) {
            write_full(fd, buf, sprintf(buf, "%s", module.supportHide ? "true" : "false"));
            close(fd);
        }

        if ((fd = OpenFile("modules", name, "api")) != -1) {
            write_full(fd, buf, sprintf(buf, "%d", module.apiVersion));
            close(fd);
        }

        if ((fd = OpenFile("modules", name, "version")) != -1) {
            write_full(fd, buf, sprintf(buf, "%d", module.version));
            close(fd);
        }

        if ((fd = OpenFile("modules", name, "version_name")) != -1) {
            write_full(fd, buf, sprintf(buf, "%s", module.versionName));
            close(fd);
        }
    }
}

int Daemon::GetMagiskVersion() {
    static int version = -1;
    if (version != -1) return version;

    int fd = -1;
    int pid = exec_command(0, &fd, "magisk", "-V", nullptr);
    if (pid == -1) {
        version = 0;
        return 0;
    }

    char buf[64];
    auto size = TEMP_FAILURE_RETRY(read(fd, buf, 64));
    close(fd);
    if (size == -1) {
        LOGE("read exec_command");
        version = 0;
        return 0;
    }

    if (buf[size - 1] == '\n') buf[size - 1] = '\0';
    version = atoi(buf);
    return version;
}

const char *Daemon::GetMagiskTmpfsPath() {
    // "magisk --path" added from v21.0, always "/sbin" in older versions
    if (GetMagiskVersion() < 21000) return "/sbin";

    static char *path = nullptr;
    if (path) return path;
    path = new char[PATH_MAX]{0};

    int fd = -1;
    int pid = exec_command(0, &fd, "magisk", "--path", nullptr);
    if (pid == -1) {
        return path;
    }
    auto size = TEMP_FAILURE_RETRY(read(fd, path, PATH_MAX));
    close(fd);
    if (size == -1) {
        return path;
    }

    if (path[size - 1] == '\n') path[size - 1] = '\0';
    return path;
}