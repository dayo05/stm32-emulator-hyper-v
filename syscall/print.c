#include "syscalls.h"


void print_hex(uint32 hex) {
    asm volatile(
            "movl %0, %%ebx \n"
            "movl %1, %%eax \n"
            "int $0x80      \n"
            :
            : "r"(hex), "i"(SYSCALL_PRINT_HEX)
            : "eax", "ebx"
            );
}

void print(char* data) {
    asm volatile(
            "movl %0, %%ebx \n"
            "movl %1, %%eax \n"
            "int $0x80      \n"
            :
            : "r"(data), "i"(SYSCALL_PRINT)
            : "eax", "ebx"
            );
}