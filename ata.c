// ata.c
#include "ports.h"
#include "ata.h"

void ata_wait_bsy() {
    while(inb(ATA_PRIMARY_COMMAND) & ATA_STATUS_BSY);
}

void ata_wait_drq() {
    while(!(inb(ATA_PRIMARY_COMMAND) & ATA_STATUS_DRQ));
}
// ata.c

// Internal function to read a SINGLE batch (max 255 sectors)
void ata_read_batch(uint32 lba, uint8 count, uint16* buffer) {
    ata_wait_bsy();

    outb(ATA_PRIMARY_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_SEC_COUNT, count);
    outb(ATA_PRIMARY_LBA_LO, (uint8)lba);
    outb(ATA_PRIMARY_LBA_MID, (uint8)(lba >> 8));
    outb(ATA_PRIMARY_LBA_HI, (uint8)(lba >> 16));
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_READ_PIO);

    // If count is 0, ATA specs say it reads 256 sectors.
    // However, to keep math simple, we will stick to 1-255 in the loop below.
    int sectors_to_read = (count == 0) ? 256 : count;

    for (int i = 0; i < sectors_to_read; i++) {
        ata_wait_bsy();
        ata_wait_drq();
        insw(ATA_PRIMARY_DATA, buffer, 256); // 256 words = 512 bytes
        buffer += 256;
    }
}

// Public function that handles ANY size
void ata_read_sectors(uint32 lba, uint32 count, void* buffer) {
    uint16* ptr = (uint16*)buffer;

    while (count > 0) {
        // We can read max 255 sectors per command safely
        uint32 chunk = (count > 255) ? 255 : count;

        ata_read_batch(lba, (uint8)chunk, ptr);

        // Update pointers and counters
        count -= chunk;
        lba   += chunk;
        ptr   += (chunk * 256); // Move pointer by (chunk * 512 bytes / 2 bytes per word)
    }
}
