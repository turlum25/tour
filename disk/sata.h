#ifndef SATA_H
#define SATA_H

#include <stdint.h>
#include <stddef.h>

#define SATA_SECTOR_SIZE          512u
#define SATA_COMMAND_IDENTIFY     0xECu

#define SATA_SIG_MID              0x3Cu
#define SATA_SIG_HIGH             0xC3u

#define AHCI_PCI_CLASS_STORAGE    0x01u
#define AHCI_PCI_SUBCLASS_AHCI    0x06u
#define AHCI_GHC_AE               (1u << 31)
#define AHCI_PORT_SIG_ATA         0x00000101u
#define AHCI_PORT_TFD_ERR         0x00000001u
#define AHCI_PORT_TFD_BUSY        0x00000080u
#define AHCI_PORT_TFD_DRQ         0x00000008u
#define AHCI_PORT_IS_TFES         (1u << 30)

typedef struct {
    uint32_t dba;
    uint32_t dbau;
    uint32_t rsvd0;
    uint32_t dbc:22;
    uint32_t rsvd1:9;
    uint32_t i:1;
} __attribute__((packed)) ahci_prdt_entry_t;

typedef struct {
    uint32_t cfis[16];
    uint32_t acmd[4];
    uint32_t rsvd[12];
    ahci_prdt_entry_t prdt;
} __attribute__((packed)) ahci_cmd_table_t;

_Static_assert(offsetof(ahci_cmd_table_t, prdt) == 128,
               "AHCI PRDT must start at byte 128");


typedef struct {
    uint32_t cfl:5;
    uint32_t a:1;
    uint32_t w:1;
    uint32_t p:1;
    uint32_t r:1;
    uint32_t b:1;
    uint32_t c:1;
    uint32_t rsvd0:1;
    uint32_t pmp:4;
    uint32_t prdtl:16;
    uint32_t prdbc;
    uint32_t ctba;
    uint32_t ctbau;
    uint32_t rsvd1[4];
} __attribute__((packed)) ahci_cmd_header_t;

typedef struct {
    uint32_t clb;
    uint32_t clbu;
    uint32_t fb;
    uint32_t fbu;
    uint32_t is;
    uint32_t ie;
    uint32_t cmd;
    uint32_t rsvd0;
    uint32_t tfd;
    uint32_t sig;
    uint32_t ssts;
    uint32_t sctl;
    uint32_t serr;
    uint32_t sact;
    uint32_t ci;
    uint32_t sntf;
    uint32_t fbs;
    uint32_t rsvd1[11];
    uint32_t vendor[4];
} __attribute__((packed)) ahci_port_t;

_Static_assert(sizeof(ahci_cmd_header_t) == 32,
               "AHCI command header must be 32 bytes");
_Static_assert(sizeof(ahci_port_t) == 128,
               "AHCI port register block must be 128 bytes");

typedef struct {
    uint32_t cap;
    uint32_t ghc;
    uint32_t is;
    uint32_t pi;
    uint32_t vs;
    uint32_t ccc_ctl;
    uint32_t ccc_pts;
    uint32_t em_loc;
    uint32_t em_ctl;
    uint32_t cap2;
    uint32_t bohc;
    uint8_t  rsvd[116];
    uint8_t  vendor[96];
    ahci_port_t ports[32];
} __attribute__((packed)) ahci_hba_mem_t;

typedef struct {
    uint16_t base_io;      
    uint16_t control_io;   
    uint8_t  slave_bit;    
    uint8_t  is_sata;      
    uint8_t  is_ahci;      
    uint32_t ahci_abar;    
    uint32_t ahci_port_idx;
} sata_drive_t;

#ifdef SATA_IMPLEMENTATION
    __attribute__((aligned(4096))) uint8_t ahci_fis_buffer[1024];
    __attribute__((aligned(1024))) uint8_t ahci_cmd_list_buffer[1024];
    __attribute__((aligned(256))) uint8_t ahci_cmd_table_buffer[512];
    sata_drive_t primary_sata_master;
#else
    extern uint8_t ahci_fis_buffer[1024];
    extern uint8_t ahci_cmd_list_buffer[1024];
    extern uint8_t ahci_cmd_table_buffer[512];
    extern sata_drive_t primary_sata_master;
#endif

static inline void sata_outb(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t sata_inb(uint16_t port) {
    uint8_t value;
    __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void sata_outl(uint16_t port, uint32_t value) {
    __asm__ __volatile__("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t sata_inl(uint16_t port) {
    uint32_t value;
    __asm__ __volatile__("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void sata_io_wait(sata_drive_t *drive) {
    sata_inb(drive->control_io);
    sata_inb(drive->control_io);
}

static inline int sata_wait_status(sata_drive_t *drive, uint8_t set_mask, uint8_t clear_mask) {
    for (uint32_t tries = 0; tries < 1000000u; ++tries) {
        uint8_t status = sata_inb(drive->base_io + 7);
        if (status == 0x00 || status == 0xFF || (status & 0x01)) {
            return 0; 
        }
        if ((status & set_mask) == set_mask && (status & clear_mask) == 0) {
            return 1;
        }
    }
    return 0;
}


static inline int sata_detect_device(sata_drive_t *drive, uint16_t base_port, uint16_t ctrl_port, uint8_t slave) {
    drive->base_io = base_port;
    drive->control_io = ctrl_port;
    drive->slave_bit = slave;
    drive->is_sata = 0;
    drive->is_ahci = 0;
    drive->ahci_abar = 0;

    for (uint32_t bus = 0; bus < 256; bus++) {
        for (uint32_t dev = 0; dev < 32; dev++) {
            for (uint32_t func = 0; func < 8; func++) { 
                uint32_t reg0 = 0;
                sata_outl(0xCF8, (0x80000000u | (bus << 16) | (dev << 11) | (func << 8) | 0x00));
                reg0 = sata_inl(0xCFC);
                if (reg0 == 0xFFFFFFFFu) continue;

                sata_outl(0xCF8, (0x80000000u | (bus << 16) | (dev << 11) | (func << 8) | 0x08));
                uint32_t class_reg = sata_inl(0xCFC);
                uint8_t base_class = (class_reg >> 24) & 0xFF;
                uint8_t subclass   = (class_reg >> 16) & 0xFF;

                if (base_class == AHCI_PCI_CLASS_STORAGE && subclass == AHCI_PCI_SUBCLASS_AHCI) {
                    sata_outl(0xCF8, (0x80000000u | (bus << 16) |
                                     (dev << 11) | (func << 8) | 0x04u));
                    uint16_t pci_command = (uint16_t)sata_inl(0xCFC);
                    sata_outl(0xCFC, (uint32_t)(pci_command | 0x0006u));

                    sata_outl(0xCF8, (0x80000000u | (bus << 16) | (dev << 11) | (func << 8) | 0x24)); // BAR5
                    uint32_t abar = sata_inl(0xCFC);
                    drive->ahci_abar = (abar & 1u) ? 0u : (abar & 0xFFFFF000u);
                    
                    if (drive->ahci_abar != 0) {
                        volatile ahci_hba_mem_t *hba = (volatile ahci_hba_mem_t *)drive->ahci_abar;
                        hba->ghc |= AHCI_GHC_AE; 
                        
                        uint32_t pi = hba->pi;
                        for (uint32_t p = 0; p < 32; p++) {
                            if (pi & (1u << p)) {
                                uint32_t ssts = hba->ports[p].ssts;
                                uint8_t ipm = (ssts >> 8) & 0x0F;
                                uint8_t det = ssts & 0x0F;
                                if (det == 3 && ipm == 1 &&
                                    hba->ports[p].sig == AHCI_PORT_SIG_ATA) {
                                    volatile ahci_port_t *port = &hba->ports[p];
                                    uint32_t tries;
                                    port->cmd &= ~0x0001u;
                                    for (tries = 0; tries < 1000000u; ++tries) {
                                        if ((port->cmd & 0x8000u) == 0) break;
                                    }
                                    if (tries == 1000000u) continue;

                                    port->cmd &= ~0x0010u;
                                    for (tries = 0; tries < 1000000u; ++tries) {
                                        if ((port->cmd & 0x4000u) == 0) break;
                                    }
                                    if (tries == 1000000u) continue;
                                    
                                    port->clb = (uint32_t)(uintptr_t)ahci_cmd_list_buffer;
                                    port->clbu = 0;
                                    port->fb  = (uint32_t)(uintptr_t)ahci_fis_buffer;
                                    port->fbu = 0;
                                    for (uint32_t i = 0; i < sizeof(ahci_cmd_list_buffer); ++i) {
                                        ahci_cmd_list_buffer[i] = 0;
                                    }
                                    for (uint32_t i = 0; i < sizeof(ahci_fis_buffer); ++i) {
                                        ahci_fis_buffer[i] = 0;
                                    }
                                    port->serr = 0xFFFFFFFFu;
                                    port->is = 0xFFFFFFFFu;
                                    port->cmd |= 0x0010u;
                                    port->cmd |= 0x0001u;
                                    drive->is_ahci = 1;
                                    drive->is_sata = 1;
                                    drive->ahci_port_idx = p;
                                    return 1;
                                }
                            }
                        }
                    }
                }
            }
        }
    }


    sata_outb(drive->base_io + 6, (uint8_t)(0xA0u | (slave << 4u)));
    sata_io_wait(drive);

    sata_outb(drive->base_io + 2, 0);
    sata_outb(drive->base_io + 3, 0);
    sata_outb(drive->base_io + 4, 0);
    sata_outb(drive->base_io + 5, 0);

    sata_outb(drive->base_io + 7, SATA_COMMAND_IDENTIFY);
    sata_io_wait(drive);

    uint8_t status = sata_inb(drive->base_io + 7);
    if (status == 0x00) return 0;

    uint8_t sig_mid = sata_inb(drive->base_io + 4);
    uint8_t sig_high = sata_inb(drive->base_io + 5);

    if (sig_mid == SATA_SIG_MID && sig_high == SATA_SIG_HIGH) {
        drive->is_sata = 1;
        return 1; 
    }

    return 0;
}


static inline int sata_read_sector(sata_drive_t *drive, uint32_t lba, uint8_t *buffer) {
    if (buffer == NULL || !drive->is_sata) return 0;

  
    if (drive->is_ahci) {
        volatile ahci_hba_mem_t *hba = (volatile ahci_hba_mem_t *)drive->ahci_abar;
        volatile ahci_port_t *port = &hba->ports[drive->ahci_port_idx];

        for (uint32_t tries = 0; port->ci != 0 && tries < 10000000u; ++tries) {
        }
        if (port->ci != 0) return 0;

        for (uint32_t tries = 0;
             (port->tfd & (AHCI_PORT_TFD_BUSY | AHCI_PORT_TFD_DRQ)) != 0 &&
             tries < 10000000u;
             ++tries) {
        }
        if ((port->tfd & (AHCI_PORT_TFD_BUSY | AHCI_PORT_TFD_DRQ)) != 0) return 0;

        port->is = 0xFFFFFFFFu;
        port->serr = 0xFFFFFFFFu;
        ahci_cmd_header_t *cmd_hdr = (ahci_cmd_header_t *)(uintptr_t)ahci_cmd_list_buffer;
        uint8_t *header_bytes = (uint8_t *)cmd_hdr;
        for (uint32_t i = 0; i < sizeof(*cmd_hdr); ++i) {
            header_bytes[i] = 0;
        }
        cmd_hdr[0].cfl = 5;
        cmd_hdr[0].w = 0;  
        cmd_hdr[0].prdtl = 1;
        
        ahci_cmd_table_t *cmd_tbl = (ahci_cmd_table_t *)(uintptr_t)ahci_cmd_table_buffer;
        uint8_t *table_bytes = (uint8_t *)cmd_tbl;
        for (uint32_t i = 0; i < sizeof(*cmd_tbl); ++i) {
            table_bytes[i] = 0;
        }
        
     
        uint8_t *fis = (uint8_t *)cmd_tbl->cfis;
        fis[0] = 0x27;    
        fis[1] = 0x80;    
        fis[2] = 0x25;      
        fis[4] = (uint8_t)lba;
        fis[5] = (uint8_t)(lba >> 8);
        fis[6] = (uint8_t)(lba >> 16);
        fis[7] = 0x40;       // LBA mode
        fis[8] = (uint8_t)(lba >> 24);
        fis[9] = 0;
        fis[10] = 0;
        fis[12] = 1;         
        fis[13] = 0;

        ahci_prdt_entry_t *prdt_ptr = (ahci_prdt_entry_t *)&cmd_tbl->prdt;
        prdt_ptr->dba = (uint32_t)(uintptr_t)buffer;
        prdt_ptr->dbc = 511;
        prdt_ptr->i = 0;



        cmd_hdr->ctba = (uint32_t)(uintptr_t)ahci_cmd_table_buffer;

        port->ci |= 1u; // Issue Command via Command Slot 0

        for (uint32_t tries = 0; tries < 10000000u; ++tries) {
            if (port->is & AHCI_PORT_IS_TFES) return 0;
            if ((port->ci & 1u) == 0) {
                return (port->tfd & (AHCI_PORT_TFD_ERR | AHCI_PORT_TFD_BUSY |
                                     AHCI_PORT_TFD_DRQ)) == 0;
            }
        }
        return 0;
    }

    if (!sata_wait_status(drive, 0, 0x80)) return 0;

    sata_outb(drive->base_io + 6, (uint8_t)(0xE0u | (drive->slave_bit << 4u) | ((lba >> 24) & 0x0Fu)));
    sata_outb(drive->base_io + 2, 1); 
    sata_outb(drive->base_io + 3, (uint8_t)lba);
    sata_outb(drive->base_io + 4, (uint8_t)(lba >> 8));
    sata_outb(drive->base_io + 5, (uint8_t)(lba >> 16));
    sata_outb(drive->base_io + 7, 0x20); 

    if (!sata_wait_status(drive, 0x08, 0x80)) return 0;

    uint16_t *words = (uint16_t *)buffer;
    for (uint32_t idx = 0; idx < 256; idx++) {
        uint16_t value;
        __asm__ __volatile__("inw %1, %0" : "=a"(value) : "Nd"(drive->base_io));
        words[idx] = value;
    }

    return 1;
}

static inline int sata_write_sector(sata_drive_t *drive,
                                    uint32_t lba,
                                    const uint8_t *buffer)
{
    if (buffer == NULL || drive == NULL || !drive->is_sata)
        return 0;

    /* Path A: AHCI */
    if (drive->is_ahci) {
        volatile ahci_hba_mem_t *hba =
            (volatile ahci_hba_mem_t *)(uintptr_t)drive->ahci_abar;
        volatile ahci_port_t *port =
            &hba->ports[drive->ahci_port_idx];

        for (uint32_t tries = 0;
             (port->ci != 0 || (port->tfd & 0x88u) != 0) &&
             tries < 10000000u;
             ++tries) {
        }

        if (port->ci != 0 || (port->tfd & 0x88u) != 0)
            return 0;

        port->is = 0xFFFFFFFFu;
        port->serr = 0xFFFFFFFFu;

        ahci_cmd_header_t *cmd_hdr =
            (ahci_cmd_header_t *)(uintptr_t)ahci_cmd_list_buffer;

        for (uint32_t i = 0; i < sizeof(*cmd_hdr); ++i)
            ((uint8_t *)cmd_hdr)[i] = 0;

        cmd_hdr->cfl = 5;
        cmd_hdr->w = 1;
        cmd_hdr->prdtl = 1;
        cmd_hdr->ctba =
            (uint32_t)(uintptr_t)ahci_cmd_table_buffer;
        cmd_hdr->ctbau = 0;

        ahci_cmd_table_t *cmd_tbl =
            (ahci_cmd_table_t *)(uintptr_t)ahci_cmd_table_buffer;

        for (uint32_t i = 0; i < sizeof(*cmd_tbl); ++i)
            ((uint8_t *)cmd_tbl)[i] = 0;

        uint8_t *fis = (uint8_t *)cmd_tbl->cfis;

        fis[0] = 0x27;
        fis[1] = 0x80;
        fis[2] = 0x35;
        fis[4] = (uint8_t)lba;
        fis[5] = (uint8_t)(lba >> 8);
        fis[6] = (uint8_t)(lba >> 16);
        fis[7] = 0x40;
        fis[8] = (uint8_t)(lba >> 24);
        fis[9] = 0;
        fis[10] = 0;
        fis[12] = 1;
        fis[13] = 0;

        ahci_prdt_entry_t *prdt = &cmd_tbl->prdt;
        prdt->dba = (uint32_t)(uintptr_t)buffer;
        prdt->dbau = 0;
        prdt->dbc = SATA_SECTOR_SIZE - 1;
        prdt->i = 0;

        port->ci |= 1u;

        for (uint32_t tries = 0; tries < 10000000u; ++tries) {
            if (port->is & AHCI_PORT_IS_TFES)
                return 0;

            if ((port->ci & 1u) == 0) {
                return (port->tfd &
                        (AHCI_PORT_TFD_ERR | AHCI_PORT_TFD_BUSY |
                         AHCI_PORT_TFD_DRQ)) == 0;
            }
        }

        return 0;
    }

    if (!sata_wait_status(drive, 0, 0x80))
        return 0;

    sata_outb(drive->base_io + 6,
              (uint8_t)(0xE0u |
                        (drive->slave_bit << 4) |
                        ((lba >> 24) & 0x0Fu)));

    sata_outb(drive->base_io + 2, 1);
    sata_outb(drive->base_io + 3, (uint8_t)lba);
    sata_outb(drive->base_io + 4, (uint8_t)(lba >> 8));
    sata_outb(drive->base_io + 5, (uint8_t)(lba >> 16));
    sata_outb(drive->base_io + 7, 0x30); 

    if (!sata_wait_status(drive, 0x08, 0x80))
        return 0;

    const uint16_t *words = (const uint16_t *)buffer;

    for (uint32_t i = 0; i < 256; ++i) {
        __asm__ __volatile__(
            "outw %0, %1"
            :
            : "a"(words[i]), "Nd"(drive->base_io)
        );
    }

    sata_io_wait(drive);

    if (!sata_wait_status(drive, 0, 0x80))
        return 0;

    return (sata_inb(drive->base_io + 7) & 0x01u) == 0;
}

#endif
