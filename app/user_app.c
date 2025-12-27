// app/user_app.c
// This is the code we will compile on Windows and inject into the VM

#include "../syscall/syscalls.h"

char* video_memory = ((char*) 0xB8000) + 320;
void user_test_interrupt() {
    *(video_memory++) = '.';
    *(video_memory++) = 0x0A;
}

__attribute__((section(".text.entry")))
void start_app() {
    // Calculate position for 3rd line (160 bytes per line)
    // 0xB8000 + (2 * 160) = 0xB8140
    int offset = 320;

    const char* msg = "[User Mode] Hello! I was loaded from the filesystem!";

    int i = 0;
    while (msg[i] != 0) {
        *(video_memory++) = msg[i];
        *(video_memory++) = 0x0A; // Light Green text
        i++;
    }

    volatile int* device = (int*)0x1F0000;
    int val = *device; // Triggers Page Fault -> Callback -> Injects 0xCAFEBABE

    // 4. Test WRITE
    *device = 0xFFFF;  // Triggers Page Fault -> Map -> Trap -> Write -> Debug -> Callback -> Print

    register_timer_function(user_test_interrupt);

    return; // Return control to the kernel
}