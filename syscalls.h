#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "kernel.h"

typedef struct {
    uint32 gs, fs, es, ds;
    uint32 edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha
    uint32 eip, cs, eflags, useresp, ss;           // Pushed by CPU on interrupt
} registers_t;

typedef void(*syscall_handler_function)(registers_t*);
void syscall_handler(registers_t *regs);

#endif