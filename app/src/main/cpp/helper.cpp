#include <jni.h>
#include <elf.h>
#include <link.h>
#include <cassert>
#include <sys/mman.h>
#include "logging.h"

extern "C" {
#include "pmparser.h"
}

static uintptr_t riru_get_version_addr;
static uintptr_t riru_get_original_native_methods_addr;
static uintptr_t riru_is_zygote_methods_replaced_addr;
static uintptr_t riru_get_nativeForkAndSpecialize_calls_count_addr;
static uintptr_t riru_get_nativeForkSystemServer_calls_count_addr;

static jboolean is_path_in_maps(const char *path) {
    procmaps_iterator *maps = pmparser_parse(-1);
    if (maps == nullptr) {
        LOGE("[map]: cannot parse the memory map");
        return JNI_FALSE;
    }

    procmaps_struct *maps_tmp = nullptr;
    while ((maps_tmp = pmparser_next(maps)) != nullptr) {
        if (strstr(maps_tmp->pathname, path))
            return JNI_TRUE;
    }
    pmparser_free(maps);
    return JNI_FALSE;
}

int xh_elf_check_elfheader(uintptr_t base_addr) {
    ElfW(Ehdr) *ehdr = (ElfW(Ehdr) *) base_addr;

    //check magic
    if (0 != memcmp(ehdr->e_ident, ELFMAG, SELFMAG)) return 1;

    //check class (64/32)
#if defined(__LP64__)
    if (ELFCLASS64 != ehdr->e_ident[EI_CLASS]) return 1;
#else
    if(ELFCLASS32 != ehdr->e_ident[EI_CLASS]) return 1;
#endif

    //check endian (little/big)
    if (ELFDATA2LSB != ehdr->e_ident[EI_DATA]) return 1;

    //check version
    if (EV_CURRENT != ehdr->e_ident[EI_VERSION]) return 1;

    //check type
    if (ET_EXEC != ehdr->e_type && ET_DYN != ehdr->e_type) return 1;

    //check machine
#if defined(__arm__)
    if(EM_ARM != ehdr->e_machine) return 1;
#elif defined(__aarch64__)
    if (EM_AARCH64 != ehdr->e_machine) return 1;
#elif defined(__i386__)
    if(EM_386 != ehdr->e_machine) return 1;
#elif defined(__x86_64__)
    if(EM_X86_64 != ehdr->e_machine) return 1;
#else
    return XH_ERRNO_FORMAT;
#endif

    //check version
    if (EV_CURRENT != ehdr->e_version) return 1;

    return 0;
}

static int init_elf(const char *pathname, uintptr_t base_in_mem) {
    struct stat statbuf{};
    int fd = open(pathname, O_RDONLY | O_CLOEXEC);
    if (fd == -1)
        return 1;

    if (fstat(fd, &statbuf) == -1)
        return 1;


    auto base = (uintptr_t) mmap(nullptr, (size_t) statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (!base)
        return 1;

    int res;
    if ((res = xh_elf_check_elfheader(base))) {
        LOGE("xh_elf_check_elfheader %d", res);
        return 1;
    }

    auto *hdr = (ElfW(Ehdr) *) base;
    auto *shdr = (ElfW(Shdr) *) (base + hdr->e_shoff);

    int sym_idx = -1;
    int dynsym_idx = -1;
    for (ElfW(Half) i = 0; i < hdr->e_shnum; i++) {
        if (shdr[i].sh_type == SHT_SYMTAB) {
            sym_idx = i;
        }
        if (shdr[i].sh_type == SHT_DYNSYM) {
            dynsym_idx = i;
        }
    }

    ElfW(Sym) *dynsyms = nullptr;
    ElfW(Xword) dynnumsyms = 0;
    uintptr_t dynstr = 0;
    if (dynsym_idx != -1) {
        dynsyms = (ElfW(Sym) *) (base + shdr[dynsym_idx].sh_offset);
        dynnumsyms = shdr[dynsym_idx].sh_size / shdr[dynsym_idx].sh_entsize;
        int dynstr_idx = shdr[dynsym_idx].sh_link;
        dynstr = base + shdr[dynstr_idx].sh_offset;
    }

    int dynsymbol_count = 0;
    if (dynsym_idx != -1) {
        // Iterate through the dynamic symbol table, and count how many symbols
        // are actually defined
        for (int i = 0; i < dynnumsyms; i++) {
            if (dynsyms[i].st_shndx != SHN_UNDEF) {
                dynsymbol_count++;
            }
        }
    }

    if (dynsym_idx != -1) {
        for (int i = 0; i < dynnumsyms; i++) {
            if (dynsyms[i].st_shndx == SHN_UNDEF)
                continue;

            if (strcmp("riru_get_version", (char *) dynstr + dynsyms[i].st_name) == 0)
                riru_get_version_addr = dynsyms[i].st_value + base_in_mem;
            else if (strcmp("riru_get_original_native_methods",
                            (char *) dynstr + dynsyms[i].st_name) == 0)
                riru_get_original_native_methods_addr = dynsyms[i].st_value + base_in_mem;
            else if (strcmp("riru_is_zygote_methods_replaced",
                            (char *) dynstr + dynsyms[i].st_name) == 0)
                riru_is_zygote_methods_replaced_addr = dynsyms[i].st_value + base_in_mem;
            else if (strcmp("riru_get_nativeForkAndSpecialize_calls_count",
                            (char *) dynstr + dynsyms[i].st_name) == 0)
                riru_get_nativeForkAndSpecialize_calls_count_addr =
                        dynsyms[i].st_value + base_in_mem;
            else if (strcmp("riru_get_nativeForkSystemServer_calls_count",
                            (char *) dynstr + dynsyms[i].st_name) == 0)
                riru_get_nativeForkSystemServer_calls_count_addr =
                        dynsyms[i].st_value + base_in_mem;
        }
    }

    munmap((void *) base, (size_t) statbuf.st_size);
    close(fd);
    return 0;
}

static jboolean init(JNIEnv *env, jobject thiz) {
    procmaps_iterator *maps = pmparser_parse(-1);
    if (maps == nullptr) {
        LOGE("[map]: cannot parse the memory map");
        return JNI_FALSE;
    }

    jboolean res = JNI_FALSE;
    procmaps_struct *maps_tmp = nullptr;
    while ((maps_tmp = pmparser_next(maps)) != nullptr) {
        if (strstr(maps_tmp->pathname, "libmemtrack.so") && maps_tmp->is_x && maps_tmp->is_p &&
            maps_tmp->is_r) {
            init_elf(maps_tmp->pathname, (uintptr_t) maps_tmp->addr_start);
        }
        if (strstr(maps_tmp->pathname, "libmemtrack_real.so")) {
            res = JNI_TRUE;
        }
    }
    pmparser_free(maps);
    return res;
}

static jboolean is_riru_module_exists(JNIEnv *env, jobject thiz, jstring name) {
    // TODO
    return JNI_FALSE;
}

static jint get_riru_rersion(JNIEnv *env, jobject thiz) {
    if (!riru_get_version_addr)
        return -1;
    return ((int (*)()) riru_get_version_addr)();
}

static jboolean is_zygote_methods_replaced(JNIEnv *env, jobject thiz) {
    if (!riru_is_zygote_methods_replaced_addr)
        return JNI_FALSE;
    return ((int (*)()) riru_is_zygote_methods_replaced_addr)() ? JNI_TRUE : JNI_FALSE;
}

static jint get_nativeForkAndSpecialize_calls_count(JNIEnv *env, jobject thiz) {
    if (!riru_get_nativeForkAndSpecialize_calls_count_addr)
        return -1;
    return ((int (*)()) riru_get_nativeForkAndSpecialize_calls_count_addr)();
}

static jint get_nativeForkSystemServer_calls_count(JNIEnv *env, jobject thiz) {
    if (!riru_get_nativeForkSystemServer_calls_count_addr)
        return -1;
    return ((int (*)()) riru_get_nativeForkSystemServer_calls_count_addr)();
}

static jstring get_nativeForkAndSpecialize_signature(JNIEnv *env, jobject thiz) {
    if (!riru_get_original_native_methods_addr)
        return nullptr;

    auto method = ((const JNINativeMethod *(*)(const char *, const char *, const char *)) riru_get_original_native_methods_addr)(
            "com/android/internal/os/Zygote", "nativeForkAndSpecialize", nullptr);
    if (method != nullptr)
        return env->NewStringUTF(method->signature);
    else
        return nullptr;
}

static jstring get_nativeForkSystemServer_signature(JNIEnv *env, jobject thiz) {
    if (!riru_get_original_native_methods_addr)
        return nullptr;

    auto method = ((const JNINativeMethod *(*)(const char *, const char *, const char *)) riru_get_original_native_methods_addr)(
            "com/android/internal/os/Zygote", "nativeForkSystemServer", nullptr);
    if (method != nullptr)
        return env->NewStringUTF(method->signature);
    else
        return nullptr;
}

static JNINativeMethod gMethods[] = {
        {"init",                                 "()Z",                   (void *) init},
        {"isRiruModuleExists",                   "(Ljava/lang/String;)Z", (void *) is_riru_module_exists},
        {"getRiruVersion",                       "()I",                   (void *) get_riru_rersion},
        {"isZygoteMethodsReplaced",              "()Z",                   (void *) is_zygote_methods_replaced},
        {"getNativeForkAndSpecializeCallsCount", "()I",                   (void *) get_nativeForkAndSpecialize_calls_count},
        {"getNativeForkSystemServerCallsCount",  "()I",                   (void *) get_nativeForkSystemServer_calls_count},
        {"getNativeForkAndSpecializeSignature",  "()Ljava/lang/String;",  (void *) get_nativeForkAndSpecialize_signature},
        {"getNativeForkSystemServerSignature",   "()Ljava/lang/String;",  (void *) get_nativeForkSystemServer_signature},
};

static int registerNativeMethods(JNIEnv *env, const char *className,
                                 JNINativeMethod *gMethods, int numMethods) {
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == nullptr)
        return JNI_FALSE;

    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0)
        return JNI_FALSE;

    return JNI_TRUE;
}

static int registerNatives(JNIEnv *env) {
    if (!registerNativeMethods(env, "moe/riru/manager/utils/NativeHelper", gMethods,
                               sizeof(gMethods) / sizeof(gMethods[0])))
        return JNI_FALSE;

    return JNI_TRUE;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = nullptr;
    jint result;

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK)
        return -1;

    assert(env != nullptr);

    if (!registerNatives(env)) {
        LOGE("registerNatives NativeHelper");
        return -1;
    }

    result = JNI_VERSION_1_6;

    return result;
}
