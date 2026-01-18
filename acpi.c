// acpi.c
#include "acpi.h"
#include "print.h" // Assuming you have print/print_hex
#include "ports.h" // Assuming inb/outb/outw defined here

// --- GLOBALS ---
uint32 PM1a_CNT = 0;
uint32 PM1b_CNT = 0;
uint16 SLP_TYPa = 0;
uint16 SLP_TYPb = 0;
uint16 SLP_EN   = 1 << 13; // ACPI standard bit to trigger the sleep

// Helper: Check checksum
int check_sdt_checksum(struct ACPISDTHeader *tableHeader) {
    unsigned char sum = 0;
    for (int i = 0; i < tableHeader->Length; i++) {
        sum += ((char *) tableHeader)[i];
    }
    return sum == 0;
}

// 1. Find the RSDP
struct RSDPDescriptor* get_rsdp() {
    // Search EBDA (0x40E << 4)
    // For simplicity in bare metal, we search the main BIOS area 0xE0000 - 0xFFFFF
    // A robust OS searches both.
    
    uint8* ptr = (uint8*)0x000E0000;
    
    for (; (uint32)ptr < 0x00100000; ptr += 16) {
        // Check for signature "RSD PTR "
        if (ptr[0] == 'R' && ptr[1] == 'S' && ptr[2] == 'D' && ptr[3] == ' ' &&
            ptr[4] == 'P' && ptr[5] == 'T' && ptr[6] == 'R' && ptr[7] == ' ') {
            
            // Validate checksum (optional but recommended)
            return (struct RSDPDescriptor*)ptr;
        }
    }
    return 0;
}

// 2. Parse DSDT to find _S5_ object (Heuristic parser)
// This scans the AML bytecode byte-by-byte looking for the sequence:
// "_S5_" (Name) -> Package Op -> Length -> Value A -> Value B
void parse_dsdt(char* ptr, int len) {
    print("Parsing DSDT for _S5_...\n");
    
    for (int i = 0; i < len - 4; i++) {
        // Look for "_S5_" signature
        if (ptr[i] == '_' && ptr[i+1] == 'S' && ptr[i+2] == '5' && ptr[i+3] == '_') {
            
            // We found the S5 object!
            // The structure in AML is usually:
            // 08 (NameOp) | 5F 53 35 5F (_S5_) | 12 (PackageOp) | ...
            
            char* s5_addr = ptr + i; 
            
            // Skip the name ("_S5_")
            s5_addr += 4;
            
            // Check for Package Opcode (0x12)
            if (*s5_addr == 0x12) {
                s5_addr++; // Skip opcode
                
                // Parse Package Length (Complex encoding, simplified here)
                // If top 2 bits are 0, it is 1 byte length.
                if ((*s5_addr & 0xC0) == 0) s5_addr++; 
                else {
                    s5_addr += ((*s5_addr >> 6) & 0x3) + 1; // Skip multi-byte length
                }
                
                s5_addr++; // Skip NumElements byte
                
                // Now we are at the SLP_TYPa value
                // Typically encoded as: 0x0A (BytePrefix) [Value] or just [Value]
                if (*s5_addr == 0x0A) s5_addr++;
                SLP_TYPa = *(s5_addr) << 10;
                
                s5_addr++; // Move to next
                
                // Now SLP_TYPb
                if (*s5_addr == 0x0A) s5_addr++;
                SLP_TYPb = *(s5_addr) << 10;
                
                print("Found _S5_ Power Off Object!\n");
                return;
            }
        }
    }
    print("ERR: _S5_ not found in DSDT.\n");
}

void init_acpi() {
    print("Searching for ACPI...\n");
    struct RSDPDescriptor* rsdp = get_rsdp();
    if (!rsdp) {
        print("ACPI RSDP not found.\n");
        return;
    }
    
    struct RSDT* rsdt = (struct RSDT*)rsdp->RsdtAddress;
    if (!check_sdt_checksum(&rsdt->h)) {
        print("RSDT Checksum failed.\n");
        return;
    }
    
    int entries = (rsdt->h.Length - sizeof(struct ACPISDTHeader)) / 4;
    
    for (int i = 0; i < entries; i++) {
        struct ACPISDTHeader* h = (struct ACPISDTHeader*)rsdt->PointerToOtherSDT[i];
        
        // Check for FADT signature
        if (h->Signature[0] == 'F' && h->Signature[1] == 'A' && 
            h->Signature[2] == 'C' && h->Signature[3] == 'P') {
            
            struct FADT* fadt = (struct FADT*)h;
            
            // 1. Enable ACPI Mode if needed
            if (fadt->SMI_CommandPort && fadt->AcpiEnable) {
                outb(fadt->SMI_CommandPort, fadt->AcpiEnable);
                // Wait a bit for transition...
                for(int w=0; w<10000; w++); 
            }
            
            // 2. Store PM Control Blocks
            PM1a_CNT = fadt->PM1a_ControlBlock;
            PM1b_CNT = fadt->PM1b_ControlBlock;
            
            // 3. Parse DSDT to get the magic numbers
            struct ACPISDTHeader* dsdt = (struct ACPISDTHeader*)fadt->Dsdt;
            parse_dsdt((char*)dsdt + sizeof(struct ACPISDTHeader), dsdt->Length);
            
            return;
        }
    }
    print("FADT not found.\n");
}

void acpi_shutdown() {
    if (PM1a_CNT == 0) {
        print("ACPI not initialized.\n");
        return;
    }
    
    print("ACPI Shutdown...\n");
    
    // Send the shutdown command
    outw(PM1a_CNT, SLP_TYPa | SLP_EN);
    
    if (PM1b_CNT != 0) {
        outw(PM1b_CNT, SLP_TYPb | SLP_EN);
    }
    
    // If we are here, it failed.
    print("Shutdown failed.");
    while(1);
}
