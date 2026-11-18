#ifndef EXT4_BRIDGE_H
#define EXT4_BRIDGE_H

#include <stdint.h>
#include "include/ext4.h"
#include "include/ext4_blockdev.h"
#include "../sata.h"

static uint8_t lw_bdev_buf[512];
static uint8_t ext4_mounted = 0;

static int sata_bdev_open(struct ext4_blockdev *bdev) {
    (void)bdev;
    return EOK;
}

static int sata_bdev_bread(struct ext4_blockdev *bdev, void *buf, uint64_t blk_id, uint32_t blk_cnt) {
    (void)bdev;
    uint8_t *dst = (uint8_t *)buf;
    for (uint32_t i = 0; i < blk_cnt; i++) {
        if (!sata_read_sector(&primary_sata_master, (uint32_t)(blk_id + i), dst + (i * 512u))) {
            return EIO;
        }
    }
    return EOK;
}

static int sata_bdev_bwrite(struct ext4_blockdev *bdev, const void *buf, uint64_t blk_id, uint32_t blk_cnt) {
    (void)bdev;
    const uint8_t *src = (const uint8_t *)buf;
    for (uint32_t i = 0; i < blk_cnt; i++) {
        if (!sata_write_sector(&primary_sata_master, (uint32_t)(blk_id + i), src + (i * 512u))) {
            return EIO;
        }
    }
    return EOK;
}

static int sata_bdev_close(struct ext4_blockdev *bdev) {
    (void)bdev;
    return EOK;
}

static struct ext4_blockdev_iface sata_iface = {
    .open     = sata_bdev_open,
    .bread    = sata_bdev_bread,
    .bwrite   = sata_bdev_bwrite,
    .close    = sata_bdev_close,
    .ph_bsize = 512,
    .ph_bcnt  = 1048576,
    .ph_bbuf  = lw_bdev_buf,
};

static struct ext4_blockdev sata_bdev = {
    .bdif        = &sata_iface,
    .part_offset = 1048576,
    .part_size   = (512ULL * 1024ULL * 1024ULL) - 1048576ULL,
};

static inline int fs_mount(void) {
    if (ext4_device_register(&sata_bdev, "sata0") != EOK) return 0;
    if (ext4_mount("sata0", "/", false) != EOK) return 0;
    ext4_mounted = 1;
    return 1;
}

static inline void fs_sync(void) {
    if (ext4_mounted) ext4_cache_flush("/");
}

static inline int fs_write(const char *path, const void *data, size_t len, int append) {
    if (!ext4_mounted) return 0;
    ext4_file f;
    if (ext4_fopen(&f, path, append ? "ab" : "wb") != EOK) return 0;
    size_t written = 0;
    int rc = ext4_fwrite(&f, data, len, &written);
    ext4_fclose(&f);
    fs_sync();
    return (rc == EOK && written == len);
}

static inline int fs_read(const char *path, char *out_buf, size_t max_len, size_t *out_read) {
    if (!ext4_mounted || max_len == 0) return 0;
    ext4_file f;
    if (ext4_fopen(&f, path, "rb") != EOK) return 0;
    size_t br = 0;
    int rc = ext4_fread(&f, out_buf, max_len - 1, &br);
    ext4_fclose(&f);
    if (rc != EOK) return 0;
    out_buf = '\0';
    if (out_read) *out_read = br;
    return 1;
}

static inline int fs_mkdir(const char *path) {
    if (!ext4_mounted) return 0;
    int rc = ext4_dir_mk(path);
    fs_sync();
    return (rc == EOK);
}

static inline int fs_rm(const char *path) {
    if (!ext4_mounted) return 0;
    int rc = ext4_fremove(path);
    fs_sync();
    return (rc == EOK);
}

#endif
