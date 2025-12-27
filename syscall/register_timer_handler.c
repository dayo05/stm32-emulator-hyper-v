#include "syscalls.h"

void register_timer_function(void (*callback_func)()) {
    // We will pass:
    // EAX = Syscall Number (1)
    // EBX = Argument 1 (The function pointer)

    asm volatile(
            "movl %0, %%ebx \n"  // Move callback pointer to EBX
            "movl %1, %%eax \n"  // Move Syscall ID to EAX
            "int $0x80      \n"  // Trigger interrupt 0x80 (The Syscall Gate)
            :
            : "r"(callback_func), "i"(SYSCALL_REGISTER_TIMER)
            : "eax", "ebx"
            );
}