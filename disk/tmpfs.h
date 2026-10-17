// Won't need this for some time

#ifndef TMPFS_H
#define TMPFS_H

#include <stdint.h>
#include <stddef.h>

#define TMPFS_MAX_DISK_FOLDERS 16u
#define TMPFS_FOLDER_NAME_MAX 32u
#define TMPFS_MAX_LBA 0x0FFFFFFFu

typedef struct {
    char name[TMPFS_FOLDER_NAME_MAX];
    uint32_t disk_lba;
} tmpfs_disk_folder_t;

static tmpfs_disk_folder_t tmpfs_disk_folders[TMPFS_MAX_DISK_FOLDERS];
static uint32_t tmpfs_disk_folder_count;
static uint8_t tmpfs_initialized;

static inline void init_tmpfs(void)
{
    for (uint32_t i = 0; i < TMPFS_MAX_DISK_FOLDERS; ++i) {
        tmpfs_disk_folders[i].name[0] = '\0';
        tmpfs_disk_folders[i].disk_lba = 0;
    }
    tmpfs_disk_folder_count = 0;
    tmpfs_initialized = 1;
}

static inline int folder(const char *name, uint32_t disk_path)
{
    if (!tmpfs_initialized || name == NULL || name[0] == '\0' || disk_path > TMPFS_MAX_LBA) {
        return 0;
    }

    uint32_t length = 0;
    while (name[length] != '\0' && length < TMPFS_FOLDER_NAME_MAX) {
        ++length;
    }
    if (length == 0 || length >= TMPFS_FOLDER_NAME_MAX) {
        return 0;
    }

    for (uint32_t i = 0; i < tmpfs_disk_folder_count; ++i) {
        uint32_t j = 0;
        while (tmpfs_disk_folders[i].name[j] == name[j] && name[j] != '\0') {
            ++j;
        }
        if (tmpfs_disk_folders[i].name[j] == '\0' && name[j] == '\0') {
            return 0;
        }
    }

    if (tmpfs_disk_folder_count >= TMPFS_MAX_DISK_FOLDERS) {
        return 0;
    }

    tmpfs_disk_folder_t *entry = &tmpfs_disk_folders[tmpfs_disk_folder_count];
    for (uint32_t i = 0; i <= length; ++i) {
        entry->name[i] = name[i];
    }
    entry->disk_lba = disk_path;
    ++tmpfs_disk_folder_count;
    return 1;
}

static inline int tmpfs_get_disk_path(const char *name, uint32_t *disk_path)
{
    if (!tmpfs_initialized || name == NULL || disk_path == NULL) {
        return 0;
    }

    for (uint32_t i = 0; i < tmpfs_disk_folder_count; ++i) {
        uint32_t j = 0;
        while (tmpfs_disk_folders[i].name[j] == name[j] && name[j] != '\0') {
            ++j;
        }
        if (tmpfs_disk_folders[i].name[j] == '\0' && name[j] == '\0') {
            *disk_path = tmpfs_disk_folders[i].disk_lba;
            return 1;
        }
    }
    return 0;
}

static inline uint32_t tmpfs_folder_count(void)
{
    return tmpfs_initialized ? tmpfs_disk_folder_count : 0;
}

static inline const char *tmpfs_folder_name(uint32_t index)
{
    if (!tmpfs_initialized || index >= tmpfs_disk_folder_count) {
        return NULL;
    }
    return tmpfs_disk_folders[index].name;
}

#endif
