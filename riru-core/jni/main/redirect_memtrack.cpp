#include <dlfcn.h>
#include <sys/types.h>

extern void init();

#ifdef __LP64__
#define MEMTRACK_LIBRARY "/system/lib64/libmemtrack_real.so"
#else
#define MEMTRACK_LIBRARY "/system/lib/libmemtrack_real.so"
#endif

static void* handle = dlopen(MEMTRACK_LIBRARY, RTLD_NOW | RTLD_GLOBAL);

#define GET_ADDRESS(NAME)                                                           \
    static void* sym_##NAME = handle ? dlsym(handle, #NAME) : NULL;                 \

GET_ADDRESS(memtrack_init) // pre-Oreo

GET_ADDRESS(memtrack_proc_destroy)
GET_ADDRESS(memtrack_proc_get)
GET_ADDRESS(memtrack_proc_gl_pss)
GET_ADDRESS(memtrack_proc_gl_total)
GET_ADDRESS(memtrack_proc_graphics_pss)
GET_ADDRESS(memtrack_proc_graphics_total)
GET_ADDRESS(memtrack_proc_new)
GET_ADDRESS(memtrack_proc_other_pss)
GET_ADDRESS(memtrack_proc_other_total)
GET_ADDRESS(_ZN7android2spINS_8hardware8memtrack4V1_09IMemtrackEED2Ev)
GET_ADDRESS(_ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE8__appendEm)

struct memtrack_proc;

extern "C" {
__attribute__((visibility("default"))) int memtrack_init(void) {
    if (!sym_memtrack_init)
        return -1;

    return ((int(*)()) sym_memtrack_init)();
}

__attribute__((visibility("default"))) struct memtrack_proc *memtrack_proc_new(void) {
    if (!sym_memtrack_proc_new)
        return NULL;

    return ((struct memtrack_proc*(*)()) sym_memtrack_proc_new)();
}

__attribute__((visibility("default"))) void memtrack_proc_destroy(struct memtrack_proc *p) {
    if (!sym_memtrack_proc_destroy)
        return;

    ((void(*)(struct memtrack_proc*)) sym_memtrack_proc_destroy)(p);
}

__attribute__((visibility("default"))) int memtrack_proc_get(struct memtrack_proc *p, pid_t pid) {
    if (!sym_memtrack_proc_get)
        return -1;

    return ((int(*)(struct memtrack_proc*, pid_t)) sym_memtrack_proc_get)(p, pid);
}

__attribute__((visibility("default"))) ssize_t memtrack_proc_graphics_total(struct memtrack_proc *p) {
    if (!sym_memtrack_proc_graphics_total)
        return 0;

    return ((ssize_t(*)(struct memtrack_proc*)) sym_memtrack_proc_graphics_total)(p);
}

__attribute__((visibility("default"))) ssize_t memtrack_proc_graphics_pss(struct memtrack_proc *p) {
    if (!sym_memtrack_proc_graphics_pss)
        return 0;

    return ((ssize_t(*)(struct memtrack_proc*)) sym_memtrack_proc_graphics_pss)(p);
}

__attribute__((visibility("default"))) ssize_t memtrack_proc_gl_total(struct memtrack_proc *p) {
    if (!sym_memtrack_proc_gl_total)
        return 0;

    return ((ssize_t(*)(struct memtrack_proc*)) sym_memtrack_proc_gl_total)(p);
}

__attribute__((visibility("default"))) ssize_t memtrack_proc_gl_pss(struct memtrack_proc *p) {
    if (!sym_memtrack_proc_gl_pss)
        return 0;

    return ((ssize_t(*)(struct memtrack_proc*)) sym_memtrack_proc_gl_pss)(p);
}

__attribute__((visibility("default"))) ssize_t memtrack_proc_other_total(struct memtrack_proc *p) {
    if (!sym_memtrack_proc_gl_total)
        return 0;

    return ((ssize_t(*)(struct memtrack_proc*)) sym_memtrack_proc_gl_total)(p);
}

__attribute__((visibility("default"))) ssize_t memtrack_proc_other_pss(struct memtrack_proc *p) {
    if (!sym_memtrack_proc_gl_pss)
        return 0;

    return ((ssize_t(*)(struct memtrack_proc*)) sym_memtrack_proc_gl_pss)(p);
}

__attribute__((visibility("default"))) int _ZN7android2spINS_8hardware8memtrack4V1_09IMemtrackEED2Ev(int res) {
    if (!sym__ZN7android2spINS_8hardware8memtrack4V1_09IMemtrackEED2Ev)
        return 0;

    return ((int(*)(int)) sym__ZN7android2spINS_8hardware8memtrack4V1_09IMemtrackEED2Ev)(res);
}

__attribute__((visibility("default"))) void _ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE8__appendEm(int* a, int b) {
    if (!sym__ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE8__appendEm)
        return;

    return ((void(*)(int *, int)) sym__ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE8__appendEm)(a, b);
}
}