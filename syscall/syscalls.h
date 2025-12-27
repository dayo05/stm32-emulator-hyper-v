#ifndef SYSCALL_SYSCALLS_H
#define SYSCALL_SYSCALLS_H

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

#define SYSCALL_REGISTER_TIMER 1
#define SYSCALL_PRINT_HEX 2
#define SYSCALL_PRINT 3

void register_timer_function(void (*callback_func)());
void print_hex(uint32);
void print(char*);
#endif