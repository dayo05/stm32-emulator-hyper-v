// acpi.h
#ifndef ACPI_H
#define ACPI_H

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

struct RSDPDescriptor {
    char Signature[8];
    uint8 Checksum;
    char OEMID[6];
    uint8 Revision;
    uint32 RsdtAddress;
} __attribute__ ((packed));

struct ACPISDTHeader {
    char Signature[4];
    uint32 Length;
    uint8 Revision;
    uint8 Checksum;
    char OEMID[6];
    char OEMTableID[8];
    uint32 OEMRevision;
    uint32 CreatorID;
    uint32 CreatorRevision;
} __attribute__ ((packed));

struct RSDT {
    struct ACPISDTHeader h;
    uint32 PointerToOtherSDT[]; 
} __attribute__ ((packed));

struct FADT {
    struct ACPISDTHeader h;
    uint32 FirmwareCtrl;
    uint32 Dsdt;
    uint8  Reserved;
    uint8  PreferredPowerManagementProfile;
    uint16 SCI_Interrupt;
    uint32 SMI_CommandPort;
    uint8  AcpiEnable;
    uint8  AcpiDisable;
    uint8  S4BIOS_REQ;
    uint8  PSTATE_CNT;
    uint32 PM1a_EventBlock;
    uint32 PM1b_EventBlock;
    uint32 PM1a_ControlBlock;
    uint32 PM1b_ControlBlock;
    // ... (There are more fields, but these are the ones we need for S5)
} __attribute__ ((packed));

void init_acpi();
void acpi_shutdown();

#endif
