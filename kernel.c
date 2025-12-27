// kernel.c

#include "kernel.h"
#include "print.h"
#include "hook.h"
#include "ata.h"
#include "ports.h"

// These functions are defined in interrupts.asm
extern void isr1_wrapper(void);
extern void isr14_wrapper(void);
extern void isr80(void);
extern void isr_timer_wrapper(void);
extern void load_idt(void* base, unsigned short size);

uint32 page_directory[1024] __attribute__((aligned(4096)));
uint32 first_page_table[1024] __attribute__((aligned(4096)));

// The actual table (256 entries)
struct IDTEntry idt[256];

// --- TIMER & CALLBACK VARIABLES ---
TimerCallback user_timer_callback = 0;
uint32 tick_counter = 0;

// --- FUNCTION DEFINITION ---
void setup_idt_entry(int n, unsigned int handler) {
    idt[n].offset_low = handler & 0xFFFF;       // Lower 16 bits of address
    idt[n].selector = 0x08;                     // Kernel Code Segment (Offset in GDT)
    idt[n].zero = 0;                            // Always zero
    idt[n].type_attr = 0x8E;                    // 10001110b (Present, Ring 0, 32-bit Interrupt Gate)
    idt[n].offset_high = (handler >> 16) & 0xFFFF; // Upper 16 bits of address
}

void remap_pic() {
    uint8 a1, a2;

    a1 = inb(0x21); // Save masks
    a2 = inb(0xA1);

    outb(0x20, 0x11); io_wait(); // Start init sequence (ICW1)
    outb(0xA0, 0x11); io_wait();

    outb(0x21, 0x20); io_wait(); // ICW2: Master Offset (0x20)
    outb(0xA1, 0x28); io_wait(); // ICW2: Slave Offset (0x28)

    outb(0x21, 0x04); io_wait(); // ICW3: Tell Master about Slave
    outb(0xA1, 0x02); io_wait(); // ICW3: Tell Slave its identity

    outb(0x21, 0x01); io_wait(); // ICW4: 8086 mode
    outb(0xA1, 0x01); io_wait();

    outb(0x21, a1);   // Restore masks
    outb(0xA1, a2);
}

// 2. Initialize PIT for 50us (20kHz)
// Base Frequency: 1193182 Hz
// Divisor = 1193182 / 20000 = ~59
void init_pit_50us() {
    uint32 frequency = 20000;
    uint32 divisor = 1193180 / frequency;

    // Command: Channel 0, Access lo/hi byte, Square Wave, 16-bit binary
    outb(0x43, 0x36);

    // Split divisor into bytes
    uint8 l = (uint8)(divisor & 0xFF);
    uint8 h = (uint8)( (divisor>>8) & 0xFF );

    // Send Frequency
    outb(0x40, l);
    outb(0x40, h);
}

// --- HANDLERS ---

// This function is called by user applications to register their handler
void register_timer_handler(TimerCallback cb) {
    user_timer_callback = cb;
    print("Timer Handler Registered!\n");
}

// This is called by the ASM wrapper (isr_timer_wrapper)
void timer_handler() {
    tick_counter++;

    // Execute user callback if it exists
    if (user_timer_callback) {
        user_timer_callback();
    }
}

// --- HOOK MANAGER ---
struct HookEntry hooks[MAX_HOOKS];
struct HookEntry* active_write_hook = 0; // Remembers which hook triggered the Trap

void register_hook(uint32 address, HookCallback cb) {
    // 1. Find empty slot
    for(int i=0; i<MAX_HOOKS; i++) {
        if (!hooks[i].active) {
            hooks[i].virtual_address = address;
            hooks[i].callback = cb;
            hooks[i].active = 1;

            // 2. Unmap the page immediately to activate the trap
            int pte_idx = address / 4096;
            // Mark Not Present (Clear Bit 0)
            first_page_table[pte_idx] &= ~1;
            asm volatile("mov %cr3, %eax; mov %eax, %cr3"); // Flush TLB

            return;
        }
    }
}

struct HookEntry* find_hook(uint32 address) {
    // We only hook aligned pages, so mask offset
    uint32 aligned_addr = address & 0xFFFFF000;
    for(int i=0; i<MAX_HOOKS; i++) {
        if (hooks[i].active && hooks[i].virtual_address == aligned_addr) {
            return &hooks[i];
        }
    }
    return 0;
}

// --- HANDLERS ---

void debug_handler(struct TrapFrame* tf) {
    // This runs AFTER the instruction executed (Single Step)

    if (active_write_hook) {
        // 1. If we were tracking a write, the data is now in RAM.
        // We call the callback so the user can see what was written.

        // Pointer to the physical page where data sits
        void* phys_mem = (void*)active_write_hook->virtual_address;

        // Invoke Callback (is_write = 1)
        active_write_hook->callback(active_write_hook->virtual_address, phys_mem, 1);

        // 2. Re-protect the page (Mark Not Present)
        int pte_idx = active_write_hook->virtual_address / 4096;
        first_page_table[pte_idx] &= ~1;
        asm volatile("mov %cr3, %eax; mov %eax, %cr3");

        // 3. Reset State
        active_write_hook = 0;
    }

    // 4. Clear Trap Flag
    tf->eflags &= ~(0x100);
}

void page_fault_handler(struct TrapFrame* tf) {
    uint32 fault_addr;
    asm volatile("mov %%cr2, %0" : "=r" (fault_addr));

    struct HookEntry* hook = find_hook(fault_addr);

    if (hook) {
        int pte_idx = hook->virtual_address / 4096;

        // Check Error Code Bit 1 (W/R): 1 = Write, 0 = Read
        int is_write_fault = (tf->error_code & 2);

        if (is_write_fault) {
            // --- WRITE INTERCEPTION ---
            // We need to let the write happen, then inspect it.

            // 1. Map Page as Writable (Present | RW)
            first_page_table[pte_idx] |= 3;
            asm volatile("mov %cr3, %eax; mov %eax, %cr3");

            // 2. Save context for the upcoming Debug Trap
            active_write_hook = hook;

            // 3. Set Trap Flag to catch CPU after the write finishes
            tf->eflags |= 0x100;

        } else {
            // --- READ INTERCEPTION ---
            // The CPU wants data. We must provide it NOW.

            // 1. Temporarily Map Page (RW so we can fill it)
            first_page_table[pte_idx] |= 3;
            asm volatile("mov %cr3, %eax; mov %eax, %cr3");

            // 2. Clear the page (optional)
            // memset((void*)hook->virtual_address, 0, 4096);

            // 3. Call Callback (is_write = 0)
            // The user function populates the memory at 'virtual_address'
            hook->callback(hook->virtual_address, (void*)hook->virtual_address, 0);

            // 4. Set Page to Read-Only (Present | User) - Clear RW Bit
            // Why Read-Only? If the user instruction was "ADD [addr], 1" (Read-Modify-Write),
            // it will Read (Trigger this), then Write (Trigger Fault again).
            // By making it Read-Only, we ensure we catch the Write part too.
            // But for simple "MOV EAX, [addr]", we can leave it Present.

            // Let's rely on Single Step to re-hide it immediately.
            active_write_hook = hook; // Reuse 'active' logic to hide page after step
            tf->eflags |= 0x100;      // Trap after instruction
        }
    } else {
        // Real Page Fault (Crash)
        print("CRASH: Invalid Access");
        while(1);
    }
}

void init_paging() {
    // 1. Identity map the first 4MB (so our kernel keeps running)
    // Fill first page table
    for (int i = 0; i < 1024; i++) {
        // Address | ReadWrite(2) | Present(1)
        first_page_table[i] = (i * 0x1000) | 3;
    }

    // 3. Set up Page Directory
    // Point the first entry to our table
    page_directory[0] = ((uint32)first_page_table) | 3;

    // Mark rest as not present
    for (int i = 1; i < 1024; i++) {
        page_directory[i] = 0;
    }

    // 4. Load CR3 (PDBR)
    asm volatile("mov %0, %%cr3" :: "r"(&page_directory));

    // 5. Enable Paging in CR0 (Bit 31)
    uint32 cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
}

// Helper: String Compare
int strcmp(const char* s1, const char* s2) {
    while(*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// Helper: Memory Copy
void memcpy(char* dest, char* src, int count) {
    for (int i=0; i<count; i++) dest[i] = src[i];
}

int secret_vault_device(uint32 address, void* page_ptr, int is_write) {
    int* data_ptr = (int*)page_ptr;

    if (is_write) {
        // The CPU just wrote to our memory. Let's see what it is.
        int val = *data_ptr;

        // (Hex conversion logic omitted for brevity, let's assume raw int print)
        print("Write Detected! New value: ");
        print_hex(val);

        // Logic: If they wrote 0xFFFF, reset the device
        if (val == 0xFFFF) {
            print("Device RESET command received");
        }

    } else {
        // The CPU is about to read. We must provide data.
        // Let's return a dynamic value (e.g., a counter)
        static int counter = 0;
        counter++;

        *data_ptr = 0xCAFEBABE + counter;

        print("Read Detected! Data injected.");
    }
    return 1;
}

void kern_main() {
    print("Loading IDT");
    // 1. Setup IDT
    setup_idt_entry(1, (uint32)isr1_wrapper);  // Debug
    setup_idt_entry(14, (uint32)isr14_wrapper); // Page Fault
    remap_pic();
    setup_idt_entry(32, (uint32)isr_timer_wrapper);
    setup_idt_entry(0x80, (uint32)isr80);
    load_idt(idt, sizeof(idt) - 1);

    // Disable IRQ14
    uint8 mask = inb(0xA1);
    mask |= (1 << 6); // Set bit 6
    outb(0xA1, mask);

    // Unmask Timer IRQ0
    outb(0x21, inb(0x21) & 0xFE);

    // 3. Configure 50us Timer
    init_pit_50us();

    // 4. Unmask IRQ0 (Timer) on PIC
    // 0xFE = 1111 1110 (Enable Bit 0 only)
    outb(0x21, inb(0x21) & 0xFE);

    // 2. Setup Paging & Hooks

    char* fs_base = (char*) 0x200000;
    // NOTE: ATA LBA 17 is exactly where we put the FS in build_fs.py
    print("Loading Filesystem...");
    ata_read_sectors(64, 2500, fs_base);
    print("Done.\n");
    init_paging();

    // 3. Parse Filesystem (Logic mostly same, just pointers changed)
    if (fs_base[0] != 'F' || fs_base[1] != 'S') {
        print("ERR: FS Magic Fail");
        while(1);
    }


    uint16 file_count = *(uint16*)(fs_base + 2);
    char* headers_start = fs_base + 4;
    char* data_start = headers_start + (file_count * sizeof(struct FileHeader));
    struct FileHeader* current_file = (struct FileHeader*) headers_start;


    register_hook(0x1F0000, secret_vault_device);

    // 5. ENABLE INTERRUPTS (Cpu-wide)
    asm volatile("sti");
    for (int i = 0; i < file_count; i++) {
        if (strcmp(current_file->name, "app.bin") == 0) {

            // 4. Relocate User App to 0x300000 (3MB Mark)
            // Giving it 1MB of dedicated space (from 3MB to 4MB)
            char* file_content = data_start + current_file->offset;
            char* execution_location = (char*)0x20000;

            memcpy(execution_location, file_content, current_file->size);

            print("Running App at 0x20000...\n");

            // 5. Execute
            FunctionPtr app_entry = (FunctionPtr)execution_location;
            app_entry();
            break;
        }
        current_file++;
    }

    print("HLT");
    while(1)
        asm volatile("hlt");
}