#include "elfext.h"

static bool elf_read(el_ctx *ctx, void *dest, size_t size, size_t offset) {
    telf_t *program = (telf_t *)ctx;
    size_t br = 0;

    if (ext4_fseek(&program->file, offset, SEEK_SET) != EOK)
        return false;

    if (ext4_fread(&program->file, dest, size, &br) != EOK)
        return false;

    return br == size;
}

int elf_open(telf_t *program, const char *path) {
    program->ctx.pread = elf_read;

    if (ext4_fopen(&program->file, path, "rb") != EOK)
        return 0;

    if (el_init(&program->ctx) != EL_OK) {
        ext4_fclose(&program->file);
        return 0;
    }

    return 1;
}

void elf_close(telf_t *program) {
    ext4_fclose(&program->file);
}

void *elf_allocate(el_ctx *ctx, Elf_Addr phys,
                   Elf_Addr virt, Elf_Addr size)
{
    (void)ctx;
    (void)phys;
    (void)size;

    return (void *)(uintptr_t)virt;
}