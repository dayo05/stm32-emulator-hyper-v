#ifndef KERNEL_H
#define KERNEL_H
// Typedefs for clarity
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

// Structure representing the stack after 'isr14_wrapper' pushes everything
struct TrapFrame {
    uint32 edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha
    uint32 error_code;                             // Pushed by CPU or Wrapper
    uint32 eip, cs, eflags;                        // Pushed by CPU
};

// Function pointer type for our app
typedef void (*FunctionPtr)();

// Filesystem structures
struct FileHeader {
    char name[16];
    uint32 size;
    uint32 offset;
} __attribute__((packed));

// --- IDT STRUCTURES ---
struct IDTEntry {
    unsigned short offset_low;
    unsigned short selector;
    unsigned char  zero;
    unsigned char  type_attr;
    unsigned short offset_high;
} __attribute__((packed));


typedef void (*TimerCallback)(void);
void register_timer_handler(TimerCallback cb);

#endif