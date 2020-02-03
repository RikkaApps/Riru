#include <zconf.h>
#include <cstring>
#include <mntent.h>
#include <cerrno>
#include <vector>
#include <android/log.h>
#include <sys/system_properties.h>
#include <dirent.h>
#include "pmparser.h"

#ifdef __LP64__
#define CHECK_LIB_NAME      "/system/lib64/libmemtrack_real.so"
#define ZYGOTE_NAME         "zygote64"
#define RESTART_NAME        "zygote_secondary"
#else
#define CHECK_LIB_NAME      "/system/lib/libmemtrack_real.so"
#define ZYGOTE_NAME         "zygote"
#define RESTART_NAME        "zygote"
#endif

#define LOG_TAG    "Riru"

#define LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...)  __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define PLOGE(fmt, args...) LOGE(fmt " failed with %d: %s", ##args, errno, strerror(errno))

static ssize_t fdgets(char *buf, const size_t size, int fd) {
    ssize_t len = 0;
    buf[0] = '\0';
    while (len < size - 1) {
        ssize_t ret = read(fd, buf + len, 1);
        if (ret < 0)
            return -1;
        if (ret == 0)
            break;
        if (buf[len] == '\0' || buf[len++] == '\n') {
            buf[len] = '\0';
            break;
        }
    }
    buf[len] = '\0';
    buf[size - 1] = '\0';
    return len;
}

static int is_proc_name_equals(int pid, const char *name) {
    int fd;

    char buf[1024];
    snprintf(buf, sizeof(buf), "/proc/%d/cmdline", pid);
    if (access(buf, R_OK) == -1 || (fd = open(buf, O_RDONLY)) == -1)
        return 0;
    if (fdgets(buf, sizeof(buf), fd) == 0) {
        snprintf(buf, sizeof(buf), "/proc/%d/comm", pid);
        close(fd);
        if (access(buf, R_OK) == -1 || (fd = open(buf, O_RDONLY)) == -1)
            return 0;
        fdgets(buf, sizeof(buf), fd);
    }
    close(fd);

    return strcmp(buf, name) == 0;
}

static int is_num(const char *s) {
    size_t len = strlen(s);
    for (size_t i = 0; i < len; ++i)
        if (s[i] < '0' || s[i] > '9')
            return 0;
    return 1;
}

static std::vector<pid_t> grep_pid(const char *name, uid_t uid) {
    DIR *dir;
    struct dirent *entry;
    std::vector<pid_t> res = std::vector<pid_t>();

    if (!(dir = opendir("/proc")))
        return res;

    struct stat st{};
    char path[32];

    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_DIR) {
            if (is_num(entry->d_name)) {
                pid_t pid = atoi(entry->d_name);
                if (is_proc_name_equals(pid, name)) {
                    sprintf(path, "/proc/%s", entry->d_name);
                    stat(path, &st);
                    if (uid == st.st_uid)
                        res.emplace_back(pid);
                }
            }
        }
    }

    closedir(dir);
    return res;
}

static int is_path_in_maps(int pid, const char *path) {
    procmaps_iterator *maps = pmparser_parse(pid);
    if (maps == nullptr) {
        LOGE("[map]: cannot parse the memory map of %d", pid);
        return false;
    }

    procmaps_struct *maps_tmp = nullptr;
    while ((maps_tmp = pmparser_next(maps)) != nullptr) {
        if (strstr(maps_tmp->pathname, path))
            return true;
    }
    pmparser_free(maps);
    return false;
}

static bool should_restart() {
    // It is said that some wired devices (Samsung? or other) have multiply zygote, get all processes called zygote
    std::vector<pid_t> pids;
    while ((pids = grep_pid(ZYGOTE_NAME, 0)).empty()) {
        LOGV(ZYGOTE_NAME " not started, wait 1s");
        sleep(1);
    }

    int count = pids.size();
    if (count > 1) {
        LOGI("multiply zygote found ?!");
    }

    int riru_count = 0;
    for (auto pid : pids) {
        if (!is_path_in_maps(pid, CHECK_LIB_NAME)) {
            LOGW("no Riru found in %s (pid=%d), restart required", ZYGOTE_NAME, pid);
        } else {
            LOGI("found Riru in %s (pid=%d)", ZYGOTE_NAME, pid);
            riru_count++;
        }
    }

    return riru_count != count;
}

static bool should_restart(int retries) {
    for (int i = 0; i < retries; ++i) {
        if (should_restart())
            return true;

        if (i != retries - 1)
            LOGV("check again after 1s, remaining %d times", retries - i - 1);

        sleep(1);
    }
    return false;
}

int main(int argc, char **argv) {
    if (fork() != 0)
        return 1;

    if (!should_restart(3))
        return 0;

    // wait for magisk mount
    while (access(CHECK_LIB_NAME, F_OK) != 0) {
        LOGV("not mounted, wait 1s");
        sleep(1);
    }

    // check again
    if (!should_restart(3)) {
        LOGI("found Riru, abort restart");
        return 0;
    }

    LOGI("restart " RESTART_NAME);
    // restart zygote_secondary will also restart zygote, see init.zygote64_32.rc
    __system_property_set("ctl.restart", const_cast<char *>(RESTART_NAME));

    return 0;
}