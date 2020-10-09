#include <sys/stat.h>
#include <fcntl.h>
#include <climits>
#include <cstdio>
#include <sys/system_properties.h>
#include <unistd.h>
#include <cstdarg>
#include "status.h"
#include "logging.h"
#include "misc.h"
#include "module.h"
#include "config.h"

#define TMP_DIR "/dev"

namespace status {

    status_t *getStatus() {
        static status_t status;
        return &status;
    }

    static const char *getRandomName() {
#ifdef __LP64__
#define DEV_RANDOM CONFIG_DIR "/dev_random64"
#else
#define DEV_RANDOM CONFIG_DIR "/dev_random"
#endif

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

    static int openFile(const char *name, ...) {
        auto random_name = getRandomName();
        if (random_name == nullptr) {
            LOGE("unable to get random name");
            return -1;
        }
        LOGD("random name is %s", random_name);

        const char *filename, *next;
        char dir[PATH_MAX];

        strcpy(dir, TMP_DIR);
#ifdef __LP64__
        strcat(dir, "/riru64_");
#else
        strcat(dir, "/riru_");
#endif
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
        if (dir_fd == -1 && mkdir(dir, 0700) == 0) {
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

#define openFile(...) openFile(__VA_ARGS__, nullptr)

    void writeToFile() {
        char buf[1024];
        int fd;

        if ((fd = openFile("api")) != -1) {
            write_full(fd, buf, sprintf(buf, "%d", RIRU_API_VERSION));
            close(fd);
        }

        if ((fd = openFile("version")) != -1) {
            write_full(fd, buf, sprintf(buf, "%d", RIRU_VERSION_CODE));
            close(fd);
        }

        if ((fd = openFile("version_name")) != -1) {
            write_full(fd, buf, sprintf(buf, "%s", RIRU_VERSION_NAME));
            close(fd);
        }

        if ((fd = openFile("hide")) != -1) {
            write_full(fd, buf, sprintf(buf, "%s", getStatus()->hideEnabled ? "true" : "false"));
            close(fd);
        }

        // write modules
        for (auto module : *get_modules()) {
            if (strcmp(module->name, MODULE_NAME_CORE) == 0) continue;
            if ((fd = openFile("modules", module->name)) != -1) {
                close(fd);
            }
        }
    }

    void writeMethodToFile(method method, bool replaced, const char *sig) {
        getStatus()->methodReplaced[method] = replaced;
        getStatus()->methodSignature[method] = strdup(sig);

        char buf[1024];
        int fd;
        if ((fd = openFile("methods", getStatus()->methodName[method])) != -1) {
            write_full(fd, buf, sprintf(buf, "%s\n%s", replaced ? "true" : "false", sig));
            close(fd);
        }
    }
}

