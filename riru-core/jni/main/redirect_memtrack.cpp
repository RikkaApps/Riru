#include <dlfcn.h>
#include <sys/types.h>

#ifdef __LP64__
#define MEMTRACK_LIBRARY "/system/lib64/libmemtrack_real.so"
#else
#define MEMTRACK_LIBRARY "/system/lib/libmemtrack_real.so"
#endif

extern "C" {
static void *handle = dlopen(MEMTRACK_LIBRARY, RTLD_NOW | RTLD_GLOBAL);

#define FUNC_DEF(NAME, RET, ...)                                                    \
    static void* sym_##NAME = handle ? dlsym(handle, #NAME) : NULL;                 \
    typedef RET (* NAME##_t)(__VA_ARGS__);                                          \
    __attribute__((visibility("default"))) RET NAME(__VA_ARGS__)

FUNC_DEF(memtrack_init, int) {// pre-Oreo
    if (!sym_memtrack_init)
        return -1;

    return ((memtrack_init_t) sym_memtrack_init)();
}

FUNC_DEF(memtrack_proc_destroy, void, struct memtrack_proc *p) {
    if (!sym_memtrack_proc_destroy)
        return;

    ((memtrack_proc_destroy_t) sym_memtrack_proc_destroy)(p);
}

FUNC_DEF(memtrack_proc_get, int, struct memtrack_proc *p, pid_t pid) {
    if (!sym_memtrack_proc_get)
        return -1;

    return ((memtrack_proc_get_t) sym_memtrack_proc_get)(p, pid);
}

FUNC_DEF(memtrack_proc_gl_pss, ssize_t, struct memtrack_proc *p) {
    if (!sym_memtrack_proc_gl_pss)
        return 0;

    return ((memtrack_proc_gl_pss_t) sym_memtrack_proc_gl_pss)(p);
}

FUNC_DEF(memtrack_proc_gl_total, ssize_t, struct memtrack_proc *p) {
    if (!sym_memtrack_proc_gl_total)
        return 0;

    return ((memtrack_proc_gl_total_t) sym_memtrack_proc_gl_total)(p);
}

FUNC_DEF(memtrack_proc_graphics_pss, ssize_t, struct memtrack_proc *p) {
    if (!sym_memtrack_proc_graphics_pss)
        return 0;

    return ((memtrack_proc_graphics_pss_t) sym_memtrack_proc_graphics_pss)(p);
}

FUNC_DEF(memtrack_proc_graphics_total, ssize_t, struct memtrack_proc *p) {
    if (!sym_memtrack_proc_graphics_total)
        return 0;

    return ((memtrack_proc_graphics_total_t) sym_memtrack_proc_graphics_total)(p);
}

FUNC_DEF(memtrack_proc_new, struct memtrack_proc *) {
    if (!sym_memtrack_proc_new)
        return NULL;

    return ((memtrack_proc_new_t) sym_memtrack_proc_new)();
}

FUNC_DEF(memtrack_proc_other_pss, ssize_t, struct memtrack_proc *p) {
    if (!sym_memtrack_proc_other_pss)
        return 0;

    return ((memtrack_proc_other_pss_t) sym_memtrack_proc_other_pss)(p);
}

FUNC_DEF(memtrack_proc_other_total, ssize_t, struct memtrack_proc *p) {
    if (!sym_memtrack_proc_other_total)
        return 0;

    return ((memtrack_proc_other_total_t) sym_memtrack_proc_other_total)(p);
}

FUNC_DEF(_ZN7android2spINS_8hardware8memtrack4V1_09IMemtrackEED2Ev, void, int a1) {
    if (!sym__ZN7android2spINS_8hardware8memtrack4V1_09IMemtrackEED2Ev)
        return;

    return ((_ZN7android2spINS_8hardware8memtrack4V1_09IMemtrackEED2Ev_t) sym__ZN7android2spINS_8hardware8memtrack4V1_09IMemtrackEED2Ev)(
            a1);
}

FUNC_DEF(_ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE8__appendEm, void, int *a1, int a2) {
    if (!sym__ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE8__appendEm)
        return;

    return ((_ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE8__appendEm_t) sym__ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE8__appendEm)(a1, a2);
}

FUNC_DEF(_ZNSt3__114__split_bufferIN7android8hardware8memtrack4V1_014MemtrackRecordERNS_9allocatorIS5_EEEC2EjjS8_, int, uint a1, uint a2, void *a3) {
    if (!sym__ZNSt3__114__split_bufferIN7android8hardware8memtrack4V1_014MemtrackRecordERNS_9allocatorIS5_EEEC2EjjS8_)
        return 0;

    return ((_ZNSt3__114__split_bufferIN7android8hardware8memtrack4V1_014MemtrackRecordERNS_9allocatorIS5_EEEC2EjjS8__t) sym__ZNSt3__114__split_bufferIN7android8hardware8memtrack4V1_014MemtrackRecordERNS_9allocatorIS5_EEEC2EjjS8_)(a1, a2, a3);
}

FUNC_DEF(_ZNSt3__113__vector_baseIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEED2Ev, void, void **a1) {
    if (!sym__ZNSt3__113__vector_baseIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEED2Ev)
        return;

    return ((_ZNSt3__113__vector_baseIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEED2Ev_t) sym__ZNSt3__113__vector_baseIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEED2Ev)(a1);
}

FUNC_DEF(_ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE8__appendEj, int, int a1, uint a2) {
    if (!sym__ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE8__appendEj)
        return 0;

    return ((_ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE8__appendEj_t) sym__ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE8__appendEj)(a1, a2);
}

FUNC_DEF(_ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE26__swap_out_circular_bufferERNS_14__split_bufferIS5_RS7_EE, int, int *a1, void *a2) {
    if (!sym__ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE26__swap_out_circular_bufferERNS_14__split_bufferIS5_RS7_EE)
        return 0;

    return ((_ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE26__swap_out_circular_bufferERNS_14__split_bufferIS5_RS7_EE_t) sym__ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE26__swap_out_circular_bufferERNS_14__split_bufferIS5_RS7_EE)(a1, a2);
}

FUNC_DEF(_ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE8allocateEj, int, void *a1, uint a2) {
    if (!sym__ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE8allocateEj)
        return 0;

    return ((_ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE8allocateEj_t) sym__ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE8allocateEj)(a1, a2);
}

FUNC_DEF(_ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE6resizeEj, int, int a1, uint a2) {
    if (!sym__ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE6resizeEj)
        return 0;

    return ((_ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE6resizeEj_t) sym__ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEE6resizeEj)(a1, a2);
}

FUNC_DEF(_ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEEC2ERKS8_, int, void *a1, void *a2) {
    if (!sym__ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEEC2ERKS8_)
        return 0;

    return ((_ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEEC2ERKS8__t) sym__ZNSt3__16vectorIN7android8hardware8memtrack4V1_014MemtrackRecordENS_9allocatorIS5_EEEC2ERKS8_)(a1, a2);
}

struct memtrack_proc;
}