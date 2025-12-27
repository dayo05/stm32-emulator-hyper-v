#include "syscalls.h"
#include "kernel.h"
#include "print.h"

void register_timer_interrupt_syscall(registers_t* regs) {
    register_timer_handler((void(*)(void))(regs->ebx));
}

void print_hex_syscall(registers_t* regs) {
    print_hex((uint32) regs->ebx);
}

void print_syscall(registers_t* regs) {
    print((char*) regs->ebx);
}

syscall_handler_function syscalls[] = {register_timer_interrupt_syscall, print_hex_syscall, print_syscall};

void syscall_handler(registers_t *regs) {
    syscalls[regs->eax - 1](regs);
}
