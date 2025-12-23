#ifndef PRINT_H
#define PRINT_H

#include "kernel.h"
void scroll();
void print_char(char c);
void clear_screen();
void print(const char* str);
void print_hex(unsigned int n);
#endif