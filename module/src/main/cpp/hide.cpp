#include <cinttypes>
#include <sys/mman.h>
#include "pmparser.h"
#include "logging.h"
#include "wrap.h"

/**
 * Magic to hide from /proc/###/maps, the idea is from Haruue Icymoon (https://github.com/haruue)
 */

#define EXPORT __attribute__((visibility("default"))) __attribute__((used))

extern "C" {
int riru_hide(const char **names, int names_count) EXPORT;
}

struct hide_struct {
    procmaps_struct *original;
    uintptr_t backup_address;
};

static int get_prot(const procmaps_struct *procstruct) {
    int prot = 0;
    if (procstruct->is_r) {
        prot |= PROT_READ;
    }
    if (procstruct->is_w) {
        prot |= PROT_WRITE;
    }
    if (procstruct->is_x) {
        prot |= PROT_EXEC;
    }
    return prot;
}

static int do_hide(hide_struct *data) {
    auto procstruct = data->original;
    auto start = (uintptr_t) procstruct->addr_start;
    auto end = (uintptr_t) procstruct->addr_end;
    auto length = end - start;
    int prot = get_prot(procstruct);

    // backup
    data->backup_address = (uintptr_t) _mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (data->backup_address == (uintptr_t) MAP_FAILED) {
        return 1;
    }
    LOGD("%" PRIxPTR"-%" PRIxPTR" %s %ld %s is backup to %" PRIxPTR, start, end, procstruct->perm, procstruct->offset, procstruct->pathname,
         data->backup_address);

    if (!procstruct->is_r) {
        _mprotect((void *) start, length, prot | PROT_READ);
    }
    memcpy((void *) data->backup_address, (void *) start, length);

    // munmap original
    munmap((void *) start, length);

    // restore
    _mmap((void *) start, length, prot, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    _mprotect((void *) start, length, prot | PROT_WRITE);
    memcpy((void *) start, (void *) data->backup_address, length);
    if (!procstruct->is_w) {
        _mprotect((void *) start, length, prot);
    }
    return 0;
}

static bool strend(char const *str, char const *suffix) {
    if (!str && !suffix) return true;
    if (!str || !suffix) return false;
    auto str_len = strlen(str);
    auto suffix_len = strlen(suffix);
    if (suffix_len > str_len) return false;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

int riru_hide(const char **names, int names_count) {
    procmaps_iterator *maps = pmparser_parse(-1);
    if (maps == nullptr) {
        LOGE("cannot parse the memory map");
        return false;
    }

    char buf[PATH_MAX];
    hide_struct *data = nullptr;
    size_t data_count = 0;
    procmaps_struct *maps_tmp;
    while ((maps_tmp = pmparser_next(maps)) != nullptr) {
        bool matched = false;
        for (int i = 0; i < names_count; ++i) {
            if (strend(maps_tmp->pathname, "/libriru.so") ||
                    (snprintf(buf, PATH_MAX, "/libriru_%s.so", names[i]) > 0 && strend(maps_tmp->pathname, buf))) {
                matched = true;
                break;
            }
        }
        if (!matched) continue;

        auto start = (uintptr_t) maps_tmp->addr_start;
        auto end = (uintptr_t) maps_tmp->addr_end;
        if (maps_tmp->is_r) {
            if (data) {
                data = (hide_struct *) realloc(data, sizeof(hide_struct) * (data_count + 1));
            } else {
                data = (hide_struct *) malloc(sizeof(hide_struct));
            }
            data[data_count].original = maps_tmp;
            data_count += 1;
        }
        LOGD("%" PRIxPTR"-%" PRIxPTR" %s %ld %s", start, end, maps_tmp->perm, maps_tmp->offset, maps_tmp->pathname);
    }

    for (int i = 0; i < data_count; ++i) {
        do_hide(&data[i]);
    }

    if (data) free(data);
    pmparser_free(maps);
    return 0;
}