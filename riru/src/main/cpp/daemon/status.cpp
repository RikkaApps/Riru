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
#include "status.h"
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

void Status::GenerateRandomName() {
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

#define OpenFile(...) OpenFile(status->is_64bit(), __VA_ARGS__, nullptr)

static void WriteCore(const Status::FbStatus *status) {
    auto core = status->core();
    if (!core) return;

    char buf[1024];
    int fd;

    if ((fd = OpenFile("api")) != -1) {
        write_full(fd, buf, sprintf(buf, "%d", core->api()));
        close(fd);
    }

    if ((fd = OpenFile("version")) != -1) {
        write_full(fd, buf, sprintf(buf, "%d", core->version()));
        close(fd);
    }

    if ((fd = OpenFile("version_name")) != -1) {
        write_full(fd, buf, sprintf(buf, "%s", core->version_name()->c_str()));
        close(fd);
    }

    if ((fd = OpenFile("hide")) != -1) {
        write_full(fd, buf, sprintf(buf, "%s", core->hide() ? "true" : "false"));
        close(fd);
    }
}

static void WriteModules(const Status::FbStatus *status) {
    auto modules = status->modules();
    if (!modules) return;

    char buf[1024];
    int fd;

    for (auto module : *modules) {
        auto name = module->name()->c_str();

        if ((fd = OpenFile("modules", name, "hide")) != -1) {
            write_full(fd, buf, sprintf(buf, "%s", module->hide() ? "true" : "false"));
            close(fd);
        }

        if ((fd = OpenFile("modules", name, "api")) != -1) {
            write_full(fd, buf, sprintf(buf, "%d", module->api()));
            close(fd);
        }

        if ((fd = OpenFile("modules", name, "version")) != -1) {
            write_full(fd, buf, sprintf(buf, "%d", module->version()));
            close(fd);
        }

        if ((fd = OpenFile("modules", name, "version_name")) != -1) {
            write_full(fd, buf, sprintf(buf, "%s", module->version_name()->c_str()));
            close(fd);
        }
    }
}

static void WriteJNIMethods(const Status::FbStatus *status) {
    auto methods = status->jni_methods();
    if (!methods) return;

    char buf[1024];
    int fd;

    for (auto method : *methods) {
        if ((fd = OpenFile("methods", method->name()->c_str())) != -1) {
            write_full(fd, buf, sprintf(buf, "%s\n%s", method->replaced() ? "true" : "false", method->signature()->c_str()));
            close(fd);
        }
    }
}

void Status::WriteToFile(const FbStatus *status) {
    WriteCore(status);
    WriteModules(status);
    WriteJNIMethods(status);
}

void Status::ReadFromFile(flatbuffers::FlatBufferBuilder &builder) {
    DIR *dir;
    struct dirent *entry;
    auto hide_enabled = access(ENABLE_HIDE_FILE, F_OK) == 0;

    std::vector<flatbuffers::Offset<Module>> modules_vector;

    if (!(dir = opendir(MODULES_DIR))) goto create_buffer;
    while ((entry = readdir(dir))) {
        if (entry->d_type != DT_DIR) continue;

        auto name = entry->d_name;
        if (name[0] == '.') continue;

        modules_vector.emplace_back(CreateModuleDirect(builder, name, 0, 0, "", false));
    }
    closedir(dir);

    create_buffer:
    auto core = CreateCoreDirect(builder, 0, 0, "", hide_enabled);

    auto modules = builder.CreateVector(modules_vector);

    FbStatusBuilder status_builder(builder);
    status_builder.add_core(core);
    status_builder.add_modules(modules);
    FinishFbStatusBuffer(builder, status_builder.Finish());
}