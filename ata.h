#ifndef ATA_H
#define ATA_H

#define ATA_PRIMARY_DATA         0x1F0
#define ATA_PRIMARY_ERR          0x1F1
#define ATA_PRIMARY_SEC_COUNT    0x1F2
#define ATA_PRIMARY_LBA_LO       0x1F3
#define ATA_PRIMARY_LBA_MID      0x1F4
#define ATA_PRIMARY_LBA_HI       0x1F5
#define ATA_PRIMARY_DRIVE_HEAD   0x1F6
#define ATA_PRIMARY_COMMAND      0x1F7
#define ATA_CMD_READ_PIO         0x20
#define ATA_STATUS_BSY           0x80
#define ATA_STATUS_DRQ           0x08

void ata_wait_bsy();

void ata_wait_drq();

// Read 'count' sectors starting at 'lba' into 'buffer'
void ata_read_sectors(uint32 lba, uint32 count, void* buffer);
#endif