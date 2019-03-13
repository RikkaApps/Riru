#include <zconf.h>
#include <cstring>
#include <mntent.h>
#include <cerrno>
#include <android/log.h>
#include <sys/system_properties.h>
#include <dirent.h>

extern "C" {
#include "pmparser.h"
}

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

static pid_t get_pid_by_name_and_uid(const char *name, uid_t uid) {
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir("/proc")))
        return -1;

    struct stat st;
    char path[32];

    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_DIR) {
            if (is_num(entry->d_name)) {
                pid_t pid = atoi(entry->d_name);
                if (is_proc_name_equals(pid, name)) {
                    sprintf(path, "/proc/%s", entry->d_name);
                    stat(path, &st);
                    if (uid == st.st_uid)
                        return pid;
                }
            }
        }
    }

    closedir(dir);
    return -1;
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

int main(int argc, char **argv) {
    if (fork() != 0)
        return 1;

    pid_t pid;
    while ((pid = get_pid_by_name_and_uid(ZYGOTE_NAME, 0)) == -1) {
        LOGV(ZYGOTE_NAME " not started, wait 1s");
        sleep(1);
    }

    if (!is_path_in_maps(pid, CHECK_LIB_NAME)) {
        LOGW("no Riru found in %s (pid=%d), restart required", ZYGOTE_NAME, pid);
    } else {
        LOGI("found Riru in %s (pid=%d)", ZYGOTE_NAME, pid);
        return 0;
    }

    // wait for magisk mount
    while (access(CHECK_LIB_NAME, F_OK) != 0) {
        LOGV("not mounted, wait 1s");
        sleep(1);
    }

    // check if zygote is restarted by other
    if ((pid = get_pid_by_name_and_uid(ZYGOTE_NAME, 0)) != -1
        && is_path_in_maps(pid, CHECK_LIB_NAME)) {
        LOGI("found Riru in %s (pid=%d), abort restart", ZYGOTE_NAME, pid);
        return 0;
    }

    LOGI("restart " RESTART_NAME);
    // restart zygote_secondary will also restart zygote, see init.zygote64_32.rc
    __system_property_set("ctl.restart", const_cast<char *>(RESTART_NAME));

    return 0;
}