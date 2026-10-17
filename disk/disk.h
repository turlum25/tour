#ifndef DISK_H
#define DISK_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "sata.h"

#define DISKLIST_MAX_ENTRIES 100u

typedef struct {
    char     name[9];
    char     ext[4];
    uint8_t  attr;         
    uint32_t first_cluster;
    uint32_t size;
} disk_fat16_dirent_t;

extern sata_drive_t primary_sata_master;
extern struct ext2context *global_ext2_ctx;

static inline int disklist(const char *directory_path,
                           disk_fat16_dirent_t entries[DISKLIST_MAX_ENTRIES])
{
    int count = 0;
    int error = 0;

    if (entries == NULL || directory_path == NULL || !ext2_volume.mounted ||
        global_ext2_ctx == NULL) {
        return -1;
    }

    void *directory = ext2_open(global_ext2_ctx, directory_path, O_RDONLY, 0, &error);
    if (directory == NULL) {
        return -1;
    }

    struct dirent *entry;
    while (count < (int)DISKLIST_MAX_ENTRIES &&
           (entry = ext2_readdir(directory, &error)) != NULL) {
        if (entry->d_name[0] == '.' &&
            (entry->d_name[1] == '\0' ||
             (entry->d_name[1] == '.' && entry->d_name[2] == '\0'))) {
            continue;
        }

        uint32_t name_length = 0;
        while (entry->d_name[name_length] != '\0' && name_length < MAXNAMLEN) {
            ++name_length;
        }
        uint32_t base_length = name_length;
        uint32_t extension_start = name_length;
        for (uint32_t i = 0; i < name_length; ++i) {
            if (entry->d_name[i] == '.') {
                base_length = i;
                extension_start = i + 1;
            }
        }

        uint32_t copied = base_length < sizeof(entries[count].name) - 1u
                              ? base_length : sizeof(entries[count].name) - 1u;
        for (uint32_t i = 0; i < copied; ++i) {
            entries[count].name[i] = entry->d_name[i];
        }
        entries[count].name[copied] = '\0';

        uint32_t extension_length = name_length - extension_start;
        copied = extension_length < sizeof(entries[count].ext) - 1u
                     ? extension_length : sizeof(entries[count].ext) - 1u;
        for (uint32_t i = 0; i < copied; ++i) {
            entries[count].ext[i] = entry->d_name[extension_start + i];
        }
        entries[count].ext[copied] = '\0';
        entries[count].first_cluster = (uint32_t)entry->d_ino;
        entries[count].attr = 0;
        entries[count].size = 0;

        char child_path[MAX_PATH_LEN];
        size_t path_length = strlen(directory_path);
        size_t child_length = path_length;
        if (path_length + name_length + 2u < sizeof(child_path)) {
            memcpy(child_path, directory_path, path_length);
            if (child_length == 0 || child_path[child_length - 1] != '/') {
                child_path[child_length++] = '/';
            }
            memcpy(child_path + child_length, entry->d_name, name_length + 1u);
            void *child = ext2_open(global_ext2_ctx, child_path, O_RDONLY, 0, &error);
            if (child != NULL) {
                struct ext2_stat info;
                if (ext2_fstat(child, &info, &error) == 0) {
                    entries[count].size = info.st_size;
                    if ((info.st_mode & 0xF000u) == 0x4000u) {
                        entries[count].attr = 0x10u;
                    }
                }
                ext2_close(child, &error);
            }
        }
        ++count;
    }

    ext2_close(directory, &error);
    return count;
}

#endif
