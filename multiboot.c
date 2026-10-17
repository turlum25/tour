#include <stdint.h>

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002u
#define MULTIBOOT_INFO_MEMORY      (1u << 0)
#define MULTIBOOT_INFO_MMAP        (1u << 6)
#define MULTIBOOT_MEMORY_AVAILABLE 1u
#define RAM_TOTAL_HANDOFF          ((volatile uint32_t *)0x7E00u)

struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t command_line;
    uint32_t modules_count;
    uint32_t modules_address;
    uint32_t symbols[4];
    uint32_t mmap_length;
    uint32_t mmap_address;
} __attribute__((packed));

struct multiboot_memory_map {
    uint32_t size;
    uint64_t address;
    uint64_t length;
    uint32_t type;
} __attribute__((packed));

void multiboot_capture_ram(uint32_t magic, uint32_t info_address)
{
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC || info_address == 0) {
        return;
    }

    const struct multiboot_info *info =
        (const struct multiboot_info *)(uintptr_t)info_address;
    uint64_t total_kilobytes = 0;

    if ((info->flags & MULTIBOOT_INFO_MMAP) != 0 &&
        info->mmap_address != 0 &&
        info->mmap_length >= sizeof(uint32_t) + 20u &&
        info->mmap_address <= UINT32_MAX - info->mmap_length) {
        uint32_t offset = 0;
        while (offset <= info->mmap_length - sizeof(uint32_t)) {
            const uint8_t *record_address =
                (const uint8_t *)(uintptr_t)(info->mmap_address + offset);
            const struct multiboot_memory_map *record =
                (const struct multiboot_memory_map *)record_address;
            uint32_t remaining = info->mmap_length - offset;

            if (remaining < sizeof(record->size) ||
                record->size < 20u ||
                record->size > remaining - sizeof(record->size)) {
                total_kilobytes = 0;
                break;
            }
            uint32_t record_bytes = record->size + sizeof(record->size);
            if (record->type == MULTIBOOT_MEMORY_AVAILABLE) {
                total_kilobytes += record->length >> 10;
            }
            offset += record_bytes;
        }
    } else if ((info->flags & MULTIBOOT_INFO_MEMORY) != 0) {
        total_kilobytes = (uint64_t)info->mem_lower + info->mem_upper;
    }

    if (total_kilobytes > UINT32_MAX) {
        total_kilobytes = UINT32_MAX;
    }
    *RAM_TOTAL_HANDOFF = (uint32_t)total_kilobytes;
}
