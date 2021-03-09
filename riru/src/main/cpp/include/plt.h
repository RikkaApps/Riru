#pragma once

#include <elf.h>
#include <link.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PLT_CHECK_PLT_APP ((unsigned short) 0x1u)
#define PLT_CHECK_PLT_ALL ((unsigned short) 0x2u)
#define PLT_CHECK_NAME    ((unsigned short) 0x4u)
#define PLT_CHECK_SYM_ONE ((unsigned short) 0x8u)

typedef struct Symbol {
    unsigned short check;
    unsigned short size;
    size_t total;
    ElfW(Addr) *symbol_plt;
    ElfW(Addr) *symbol_sym;
    const char *symbol_name;
    char **names;
} Symbol;

int dl_iterate_phdr_symbol(Symbol *symbol);

void *plt_dlsym(const char *name, size_t *total);

#ifdef __cplusplus
}
#endif