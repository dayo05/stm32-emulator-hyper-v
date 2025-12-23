// ports.h
#ifndef PORTS_H
#define PORTS_H

#include "kernel.h"

static inline uint8 inb(uint16 port) {
    uint8 result;
    asm volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outb(uint16 port, uint8 data) {
    asm volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

static inline void insw(uint16 port, void* addr, int count) {
    asm volatile("rep insw" : "+D"(addr), "+c"(count) : "d"(port) : "memory");
}

#endif