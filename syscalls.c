#include "syscalls.h"
#include "kernel.h"
#include "print.h"

void register_timer_interrupt_syscall(registers_t* regs) {
    register_timer_handler((void(*)(void))(regs->ebx));
}

syscall_handler_function syscalls[] = {register_timer_interrupt_syscall};

void syscall_handler(registers_t *regs) {
    print("SYSCALL with ");
    print_hex(regs->eax);
    print("Recved");
    syscalls[regs->eax - 1](regs);
}
