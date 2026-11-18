#ifndef ELFEXT_H
#define ELFEXT_H

#include "elfload.h"
#include "../disk/ext4/ext4.h"

typedef struct {
    el_ctx ctx;
    ext4_file file;
} telf_t;

int elf_open(telf_t *program, const char *path);
void elf_close(telf_t *program);
void *elf_allocate(el_ctx *ctx, Elf_Addr phys, Elf_Addr virt, Elf_Addr size);

#endif