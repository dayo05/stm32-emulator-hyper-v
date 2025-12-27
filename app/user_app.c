// app/user_app.c
// This is the code we will compile on Windows and inject into the VM

#include "../syscall/syscalls.h"


void user_test_interrupt() {
    print(".");
    // *(video_memory++) = '.';
    // *(video_memory++) = 0x0A;
}

__attribute__((section(".text.entry")))
void start_app() {

    print("[User Mode] Hello! I was loaded from the filesystem!\n");

    volatile int* device = (int*)0x1F0000;
    int val = *device; // Triggers Page Fault -> Callback -> Injects 0xCAFEBABE

    // 4. Test WRITE
    *device = 0xFFFF;  // Triggers Page Fault -> Map -> Trap -> Write -> Debug -> Callback -> Print

    register_timer_function(user_test_interrupt);

    return; // Return control to the kernel
}