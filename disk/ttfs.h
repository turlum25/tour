// TTFS (TOUR's Terrible File System)
// Oct 27, 2026
// Utter garbage innit. Made idea, vibecoded it. Tho i would still use 
#ifndef TTFS_H
#define TTFS_H
#include "sata.h"

// Sectors 0-10 are reserved. The index occupies sectors 11-2048.
#define TTFS_RESERVED_SECTORS     11u
#define TTFS_INDEX_FIRST_SECTOR   11u
#define TTFS_INDEX_LAST_SECTOR    2048u
#define TTFS_DATA_FIRST_SECTOR    2049u
#define TTFS_MAX_LBA              0x0FFFFFFFu

#define TTFS_FILENAME_SIZE       32u
#define TTFS_ENTRY_SIZE          40u
#define TTFS_ENTRIES_PER_SECTOR  12u
#define TTFS_MAX_FILES           ((TTFS_INDEX_LAST_SECTOR - TTFS_INDEX_FIRST_SECTOR + 1u) * TTFS_ENTRIES_PER_SECTOR)
#define TTFS_INDEX_SECTOR_COUNT  (TTFS_INDEX_LAST_SECTOR - TTFS_INDEX_FIRST_SECTOR + 1u)

static uint8_t ttfs_ready = 0;

static void ttfs_zero(uint8_t *buffer, uint32_t size) {
    for (uint32_t i = 0; i < size; ++i) {
        buffer[i] = 0;
    }
}

static uint32_t ttfs_read_u32(const uint8_t *bytes) {
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static uint32_t ttfs_sector_count(uint32_t size);

static void ttfs_write_u32(uint8_t *bytes, uint32_t value) {
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
}

static uint32_t ttfs_slot_offset(uint32_t slot) {
    uint32_t sector_offset = slot / TTFS_ENTRIES_PER_SECTOR;
    uint32_t local_slot = slot % TTFS_ENTRIES_PER_SECTOR;
    uint32_t offset = local_slot * TTFS_ENTRY_SIZE;
    return (sector_offset == 0) ? (offset + 8u) : offset;
}

static int ttfs_slot_valid(uint32_t slot) {
    return slot < TTFS_MAX_FILES;
}

static int ttfs_index_valid(const uint8_t *sector) {
    return sector[0] == 'T' && sector[1] == 'T' &&
           sector[2] == 'F' && sector[3] == 'S' && sector[4] == 1;
}

static int ttfs_entry_valid(const uint8_t *entry) {
    uint32_t name_length = 0;
    while (name_length < TTFS_FILENAME_SIZE && entry[name_length] != '\0') {
        ++name_length;
    }
    if (name_length == TTFS_FILENAME_SIZE) {
        return 0;
    }
    if (name_length == 0) {
        return 1;
    }

    uint32_t start = ttfs_read_u32(entry + TTFS_FILENAME_SIZE);
    uint32_t size = ttfs_read_u32(entry + TTFS_FILENAME_SIZE + 4u);
    uint32_t sectors = ttfs_sector_count(size);
    if (sectors == 0) {
        return start == 0;
    }
    return start >= TTFS_DATA_FIRST_SECTOR &&
           sectors - 1u <= TTFS_MAX_LBA &&
           start <= TTFS_MAX_LBA - (sectors - 1u);
}

static int ttfs_read_entry(uint32_t slot, uint8_t *entry) {
    if (!ttfs_slot_valid(slot) || !entry) {
        return 0;
    }
    uint8_t sector[SATA_SECTOR_SIZE];
    uint32_t sector_offset = slot / TTFS_ENTRIES_PER_SECTOR;
    if (!sata_read_sector(&primary_sata_master,
                          TTFS_INDEX_FIRST_SECTOR + sector_offset, sector)) {
        return 0;
    }
    uint32_t offset = ttfs_slot_offset(slot);
    for (uint32_t i = 0; i < TTFS_ENTRY_SIZE; ++i) {
        entry[i] = sector[offset + i];
    }
    return 1;
}

static int ttfs_write_entry(uint32_t slot, const uint8_t *entry) {
    if (!ttfs_slot_valid(slot) || !entry || !ttfs_entry_valid(entry)) {
        return 0;
    }
    uint8_t sector[SATA_SECTOR_SIZE];
    uint32_t sector_offset = slot / TTFS_ENTRIES_PER_SECTOR;
    if (!sata_read_sector(&primary_sata_master,
                          TTFS_INDEX_FIRST_SECTOR + sector_offset, sector)) {
        return 0;
    }
    uint32_t offset = ttfs_slot_offset(slot);
    for (uint32_t i = 0; i < TTFS_ENTRY_SIZE; ++i) {
        sector[offset + i] = entry[i];
    }
    return sata_write_sector(&primary_sata_master,
                             TTFS_INDEX_FIRST_SECTOR + sector_offset, sector);
}

static int ttfs_format_index(void) {
    uint8_t sector[SATA_SECTOR_SIZE];
    ttfs_zero(sector, SATA_SECTOR_SIZE);
    sector[0] = 'T';
    sector[1] = 'T';
    sector[2] = 'F';
    sector[3] = 'S';
    sector[4] = 1;
    return sata_write_sector(&primary_sata_master,
                             TTFS_INDEX_FIRST_SECTOR, sector);
}

static int ttfs_index_is_blank(void) {
    uint8_t sector[SATA_SECTOR_SIZE];
    for (uint32_t i = 0; i < TTFS_INDEX_SECTOR_COUNT; ++i) {
        if (!sata_read_sector(&primary_sata_master,
                              TTFS_INDEX_FIRST_SECTOR + i, sector)) {
            return 0;
        }
        for (uint32_t byte = 0; byte < SATA_SECTOR_SIZE; ++byte) {
            if (sector[byte] != 0) {
                return 0;
            }
        }
    }
    return 1;
}

static int ttfs_valid_name(const char *filename) {
    if (!filename || !filename[0]) {
        return 0;
    }
    for (uint32_t i = 0; i < TTFS_FILENAME_SIZE; ++i) {
        if (filename[i] == '\0') {
            return 1;
        }
    }
    return 0;
}

static int ttfs_scan(const char *filename, uint32_t *found_slot,
                     uint32_t *free_slot) {
    uint8_t sector[SATA_SECTOR_SIZE];
    int found = -1;
    int first_free = -1;
    for (uint32_t sector_offset = 0;
         sector_offset < TTFS_INDEX_SECTOR_COUNT; ++sector_offset) {
        if (!sata_read_sector(&primary_sata_master,
                              TTFS_INDEX_FIRST_SECTOR + sector_offset, sector)) {
            return 0;
        }
        uint32_t first_slot = sector_offset * TTFS_ENTRIES_PER_SECTOR;
        if (sector_offset == 0 && !ttfs_index_valid(sector)) {
            return 0;
        }
        for (uint32_t i = 0; i < TTFS_ENTRIES_PER_SECTOR; ++i) {
            uint32_t slot = first_slot + i;
            uint8_t *entry = &sector[ttfs_slot_offset(slot)];
            if (entry[0] == '\0') {
                if (first_free < 0) {
                    first_free = (int)slot;
                }
                continue;
            }
            if (!ttfs_entry_valid(entry)) {
                return 0;
            }
            if (filename) {
                uint32_t n = 0;
                while (n < TTFS_FILENAME_SIZE && filename[n] &&
                       entry[n] == (uint8_t)filename[n]) {
                    ++n;
                }
                if (n < TTFS_FILENAME_SIZE && filename[n] == '\0' &&
                    entry[n] == '\0') {
                    if (found >= 0) {
                        return 0;
                    }
                    found = (int)slot;
                }
            }
        }
    }
    if (found_slot) {
        *found_slot = found < 0 ? UINT32_MAX : (uint32_t)found;
    }
    if (free_slot) {
        *free_slot = first_free < 0 ? UINT32_MAX : (uint32_t)first_free;
    }
    return 1;
}

static uint32_t ttfs_sector_count(uint32_t size) {
    return size / SATA_SECTOR_SIZE + (size % SATA_SECTOR_SIZE != 0);
}

static int ttfs_find_data_run(uint32_t needed, uint32_t *start_sector,
                              int ignored_slot) {
    uint32_t candidate = TTFS_DATA_FIRST_SECTOR;

    if (needed == 0 || !start_sector ||
        needed - 1u > TTFS_MAX_LBA - TTFS_DATA_FIRST_SECTOR) {
        return 0;
    }

    while (candidate <= TTFS_MAX_LBA - (needed - 1u)) {
        int moved = 0;
        uint8_t sector[SATA_SECTOR_SIZE];
        for (uint32_t sector_offset = 0;
             sector_offset < TTFS_INDEX_SECTOR_COUNT && !moved;
             ++sector_offset) {
            if (!sata_read_sector(&primary_sata_master,
                                  TTFS_INDEX_FIRST_SECTOR + sector_offset,
                                  sector)) {
                return 0;
            }
            uint32_t first_slot = sector_offset * TTFS_ENTRIES_PER_SECTOR;
            for (uint32_t i = 0; i < TTFS_ENTRIES_PER_SECTOR; ++i) {
                uint32_t slot = first_slot + i;
                if ((int)slot == ignored_slot) {
                    continue;
                }
                uint8_t *entry = &sector[ttfs_slot_offset(slot)];
                if (entry[0] == '\0') {
                    continue;
                }
                if (!ttfs_entry_valid(entry)) {
                    return 0;
                }

                uint32_t file_start = ttfs_read_u32(entry + TTFS_FILENAME_SIZE);
                uint32_t file_sectors = ttfs_sector_count(
                    ttfs_read_u32(entry + TTFS_FILENAME_SIZE + 4u));
                if (file_sectors == 0) {
                    continue;
                }
                if (candidate < file_start + file_sectors &&
                    candidate + needed > file_start) {
                    candidate = file_start + file_sectors;
                    moved = 1;
                    break;
                }
            }
        }
        if (!moved) {
            *start_sector = candidate;
            return 1;
        }
    }
    return 0;
}

int fsinit(void) {
    uint8_t sector[SATA_SECTOR_SIZE];
    ttfs_ready = 0;

    if (!sata_detect_device(&primary_sata_master, 0x1F0, 0x3F6, 0)) {
        return 0;
    }
    if (!sata_read_sector(&primary_sata_master,
                          TTFS_INDEX_FIRST_SECTOR, sector)) {
        return 0;
    }

    if (!ttfs_index_valid(sector) &&
        (!ttfs_index_is_blank() || !ttfs_format_index())) {
        return 0;
    }
    ttfs_ready = 1;
    return 1;
}

int fsready(void) {
    return ttfs_ready;
}

int fsfilesize(const char *filename, uint32_t *size) {
    uint32_t slot;
    uint8_t entry[TTFS_ENTRY_SIZE];
    if (!ttfs_ready || !ttfs_valid_name(filename) || !size ||
        !ttfs_scan(filename, &slot, NULL) || slot == UINT32_MAX ||
        !ttfs_read_entry(slot, entry) || !ttfs_entry_valid(entry) ||
        entry[0] == '\0') {
        return 0;
    }

    *size = ttfs_read_u32(entry + TTFS_FILENAME_SIZE + 4u);
    return 1;
}

int fsread(const char *filename, uint8_t *buffer, uint32_t buffer_size,
           uint32_t *bytes_read) {
    uint32_t slot;
    uint8_t entry[TTFS_ENTRY_SIZE];
    if (bytes_read) {
        *bytes_read = 0;
    }
    if (!ttfs_ready || !ttfs_valid_name(filename) || !bytes_read ||
        !ttfs_scan(filename, &slot, NULL) ||
        slot == UINT32_MAX || !ttfs_read_entry(slot, entry)) {
        return 0;
    }
    if (!ttfs_entry_valid(entry) || entry[0] == '\0') {
        return 0;
    }
    uint32_t size = ttfs_read_u32(entry + TTFS_FILENAME_SIZE + 4u);
    uint32_t start = ttfs_read_u32(entry + TTFS_FILENAME_SIZE);
    if (buffer_size < size || (size && !buffer)) {
        return 0;
    }

    uint32_t remaining = size;
    uint32_t sector_index = 0;
    uint8_t sector[SATA_SECTOR_SIZE];
    while (remaining) {
        if (!sata_read_sector(&primary_sata_master, start + sector_index, sector)) {
            return 0;
        }
        uint32_t bytes = remaining < SATA_SECTOR_SIZE ? remaining : SATA_SECTOR_SIZE;
        for (uint32_t i = 0; i < bytes; ++i) {
            buffer[sector_index * SATA_SECTOR_SIZE + i] = sector[i];
        }
        remaining -= bytes;
        ++sector_index;
    }
    *bytes_read = size;
    return 1;
}

typedef int (*ttfs_read_callback_t)(const uint8_t *data, uint32_t size,
                                    void *context);

int fsread_stream(const char *filename, ttfs_read_callback_t callback,
                  void *context) {
    uint32_t slot;
    uint8_t entry[TTFS_ENTRY_SIZE];
    if (!ttfs_ready || !ttfs_valid_name(filename) || !callback ||
        !ttfs_scan(filename, &slot, NULL) || slot == UINT32_MAX ||
        !ttfs_read_entry(slot, entry) || !ttfs_entry_valid(entry) ||
        entry[0] == '\0') {
        return 0;
    }

    uint32_t remaining = ttfs_read_u32(entry + TTFS_FILENAME_SIZE + 4u);
    uint32_t start = ttfs_read_u32(entry + TTFS_FILENAME_SIZE);
    uint32_t sector_index = 0;
    uint8_t sector[SATA_SECTOR_SIZE];
    while (remaining) {
        if (!sata_read_sector(&primary_sata_master, start + sector_index, sector)) {
            return 0;
        }
        uint32_t bytes = remaining < SATA_SECTOR_SIZE
                             ? remaining
                             : SATA_SECTOR_SIZE;
        if (!callback(sector, bytes, context)) {
            return 0;
        }
        remaining -= bytes;
        ++sector_index;
    }
    return 1;
}

int fswrite(const char *filename, const uint8_t *buffer, uint32_t size) {
    uint32_t slot;
    uint32_t free_slot;
    uint8_t entry[TTFS_ENTRY_SIZE];
    if (!ttfs_ready || !ttfs_valid_name(filename) || (!buffer && size) ||
        !ttfs_scan(filename, &slot, &free_slot)) {
        return 0;
    }

    int existing_slot = slot == UINT32_MAX ? -1 : (int)slot;
    if (slot == UINT32_MAX) {
        slot = free_slot;
    }
    if (slot == UINT32_MAX) {
        return 0;
    }

    if (existing_slot >= 0 &&
        (!ttfs_read_entry((uint32_t)existing_slot, entry) ||
         !ttfs_entry_valid(entry) || entry[0] == '\0')) {
        return 0;
    }

    uint32_t start = 0;
    uint32_t sectors = ttfs_sector_count(size);
    if (sectors) {
        uint8_t *old_entry = existing_slot >= 0 ? entry : NULL;
        uint32_t old_start = old_entry
                                 ? ttfs_read_u32(old_entry + TTFS_FILENAME_SIZE)
                                 : 0;
        uint32_t old_sectors = old_entry
                                   ? ttfs_sector_count(ttfs_read_u32(
                                         old_entry + TTFS_FILENAME_SIZE + 4u))
                                   : 0;
        if (old_sectors >= sectors && old_start >= TTFS_DATA_FIRST_SECTOR) {
            start = old_start;
        } else if (!ttfs_find_data_run(sectors, &start, -1)) {
            return 0;
        }
    }

    uint32_t remaining = size;
    uint32_t sector_index = 0;
    uint8_t sector[SATA_SECTOR_SIZE];
    while (remaining) {
        ttfs_zero(sector, SATA_SECTOR_SIZE);
        uint32_t bytes = remaining < SATA_SECTOR_SIZE ? remaining : SATA_SECTOR_SIZE;
        for (uint32_t i = 0; i < bytes; ++i) {
            sector[i] = buffer[sector_index * SATA_SECTOR_SIZE + i];
        }
        if (!sata_write_sector(&primary_sata_master, start + sector_index, sector)) {
            return 0;
        }
        remaining -= bytes;
        ++sector_index;
    }

    ttfs_zero(entry, TTFS_ENTRY_SIZE);
    for (uint32_t i = 0; filename[i]; ++i) {
        entry[i] = (uint8_t)filename[i];
    }
    ttfs_write_u32(entry + TTFS_FILENAME_SIZE, start);
    ttfs_write_u32(entry + TTFS_FILENAME_SIZE + 4u, size);

    return ttfs_write_entry(slot, entry);
}

int fslist(char names[][TTFS_FILENAME_SIZE], uint32_t capacity,
           uint32_t *file_count) {
    uint8_t sector[SATA_SECTOR_SIZE];
    if (file_count) {
        *file_count = 0;
    }
    if (!ttfs_ready || !file_count ||
        !sata_read_sector(&primary_sata_master, TTFS_INDEX_FIRST_SECTOR, sector) ||
        !ttfs_index_valid(sector)) {
        return 0;
    }

    uint32_t count = 0;
    for (uint32_t sector_offset = 0;
         sector_offset < TTFS_INDEX_SECTOR_COUNT; ++sector_offset) {
        if (!sata_read_sector(&primary_sata_master,
                              TTFS_INDEX_FIRST_SECTOR + sector_offset, sector)) {
            return 0;
        }
        for (uint32_t i = 0; i < TTFS_ENTRIES_PER_SECTOR; ++i) {
            uint32_t slot = sector_offset * TTFS_ENTRIES_PER_SECTOR + i;
            uint8_t *entry = &sector[ttfs_slot_offset(slot)];
            if (entry[0] != '\0') {
                if (!ttfs_entry_valid(entry)) {
                    return 0;
                }
                ++count;
            }
        }
    }
    *file_count = count;
    if (capacity < count || (count && !names)) {
        return 0;
    }

    uint32_t out = 0;
    for (uint32_t sector_offset = 0;
         sector_offset < TTFS_INDEX_SECTOR_COUNT; ++sector_offset) {
        if (!sata_read_sector(&primary_sata_master,
                              TTFS_INDEX_FIRST_SECTOR + sector_offset, sector)) {
            return 0;
        }
        for (uint32_t i = 0; i < TTFS_ENTRIES_PER_SECTOR; ++i) {
            uint32_t slot = sector_offset * TTFS_ENTRIES_PER_SECTOR + i;
            uint8_t *entry = &sector[ttfs_slot_offset(slot)];
            if (entry[0] == '\0') {
                continue;
            }
            if (!ttfs_entry_valid(entry)) {
                return 0;
            }
            for (uint32_t n = 0; n < TTFS_FILENAME_SIZE; ++n) {
                names[out][n] = (char)entry[n];
            }
            ++out;
        }
    }
    return 1;
}

typedef struct {
    char name[TTFS_FILENAME_SIZE];
    uint32_t size;
    uint8_t is_directory;
} ttfs_dirent_t;

static int ttfs_directory_name_valid(const char *directory) {
    if (!directory) {
        return 0;
    }
    uint32_t length = 0;
    uint32_t component_length = 0;
    while (directory[length]) {
        if (length >= TTFS_FILENAME_SIZE - 1u) {
            return 0;
        }
        if (directory[length] == '/') {
            if (component_length == 0) {
                return 0;
            }
            component_length = 0;
        } else {
            ++component_length;
        }
        ++length;
    }
    return component_length != 0 || length == 0;
}

int fsdir_exists(const char *directory) {
    if (!ttfs_ready || !ttfs_directory_name_valid(directory)) {
        return 0;
    }
    if (directory[0] == '\0') {
        return 1;
    }

    uint32_t directory_length = 0;
    while (directory[directory_length]) {
        ++directory_length;
    }

    uint8_t sector[SATA_SECTOR_SIZE];
    for (uint32_t sector_offset = 0;
         sector_offset < TTFS_INDEX_SECTOR_COUNT; ++sector_offset) {
        if (!sata_read_sector(&primary_sata_master,
                              TTFS_INDEX_FIRST_SECTOR + sector_offset, sector)) {
            return 0;
        }
        if (sector_offset == 0 && !ttfs_index_valid(sector)) {
            return 0;
        }
        uint32_t first_slot = sector_offset * TTFS_ENTRIES_PER_SECTOR;
        for (uint32_t i = 0; i < TTFS_ENTRIES_PER_SECTOR; ++i) {
            uint8_t *entry = &sector[ttfs_slot_offset(first_slot + i)];
            if (entry[0] == '\0') {
                continue;
            }
            if (!ttfs_entry_valid(entry)) {
                return 0;
            }
            uint32_t name_length = 0;
            while (name_length < TTFS_FILENAME_SIZE && entry[name_length]) {
                ++name_length;
            }
            if (name_length > directory_length &&
                entry[directory_length] == '/') {
                uint32_t n = 0;
                while (n < directory_length &&
                       entry[n] == (uint8_t)directory[n]) {
                    ++n;
                }
                if (n == directory_length) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

int fslist_dir(const char *directory, ttfs_dirent_t *entries,
               uint32_t capacity, uint32_t *entry_count) {
    if (entry_count) {
        *entry_count = 0;
    }
    if (!ttfs_ready || !ttfs_directory_name_valid(directory) || !entry_count ||
        (capacity && !entries)) {
        return 0;
    }

    uint32_t directory_length = 0;
    while (directory[directory_length]) {
        ++directory_length;
    }

    uint32_t found = 0;
    uint8_t sector[SATA_SECTOR_SIZE];
    for (uint32_t sector_offset = 0;
         sector_offset < TTFS_INDEX_SECTOR_COUNT; ++sector_offset) {
        if (!sata_read_sector(&primary_sata_master,
                              TTFS_INDEX_FIRST_SECTOR + sector_offset, sector)) {
            return 0;
        }
        if (sector_offset == 0 && !ttfs_index_valid(sector)) {
            return 0;
        }

        uint32_t first_slot = sector_offset * TTFS_ENTRIES_PER_SECTOR;
        for (uint32_t i = 0; i < TTFS_ENTRIES_PER_SECTOR; ++i) {
            uint8_t *entry = &sector[ttfs_slot_offset(first_slot + i)];
            if (entry[0] == '\0') {
                continue;
            }
            if (!ttfs_entry_valid(entry)) {
                return 0;
            }

            uint32_t path_length = 0;
            while (path_length < TTFS_FILENAME_SIZE && entry[path_length]) {
                ++path_length;
            }
            uint32_t component_start = 0;
            if (directory_length) {
                if (path_length <= directory_length ||
                    entry[directory_length] != '/') {
                    continue;
                }
                uint32_t n = 0;
                while (n < directory_length &&
                       entry[n] == (uint8_t)directory[n]) {
                    ++n;
                }
                if (n != directory_length) {
                    continue;
                }
                component_start = directory_length + 1u;
            }

            if (component_start >= path_length) {
                continue;
            }
            uint32_t component_length = component_start;
            while (component_length < path_length &&
                   entry[component_length] != '/') {
                ++component_length;
            }
            uint32_t name_length = component_length - component_start;
            int is_directory = component_length < path_length;

            uint32_t existing = 0;
            while (existing < found) {
                uint32_t n = 0;
                while (n < name_length &&
                       entries[existing].name[n] ==
                           (char)entry[component_start + n]) {
                    ++n;
                }
                if (n == name_length && entries[existing].name[n] == '\0') {
                    break;
                }
                ++existing;
            }
            if (existing < found) {
                if (is_directory) {
                    entries[existing].is_directory = 1;
                }
                continue;
            }
            if (found >= capacity) {
                *entry_count = found + 1u;
                return 0;
            }

            for (uint32_t n = 0; n < name_length; ++n) {
                entries[found].name[n] = (char)entry[component_start + n];
            }
            entries[found].name[name_length] = '\0';
            entries[found].size = is_directory
                                      ? 0
                                      : ttfs_read_u32(entry + TTFS_FILENAME_SIZE + 4u);
            entries[found].is_directory = (uint8_t)is_directory;
            ++found;
        }
    }

    *entry_count = found;
    return 1;
}

int fsdelete(const char *filename) {
    uint32_t slot;
    uint8_t entry[TTFS_ENTRY_SIZE];
    if (!ttfs_ready || !ttfs_valid_name(filename) ||
        !ttfs_scan(filename, &slot, NULL) || slot == UINT32_MAX ||
        !ttfs_read_entry(slot, entry) || !ttfs_entry_valid(entry) ||
        entry[0] == '\0') {
        return 0;
    }

    ttfs_zero(entry, TTFS_ENTRY_SIZE);
    return ttfs_write_entry(slot, entry);
}



#endif // TTFS_H