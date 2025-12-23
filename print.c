// print.c

#include "print.h"

#define VGA_ADDRESS 0xB8000
#define VGA_COLS 80
#define VGA_ROWS 25
#define WHITE_ON_BLACK 0x0F

// Global Cursor Position
int cursor_x = 0;
int cursor_y = 0;

// Helper: Scroll the screen up by one line
void scroll() {
    // 1. Calculate the blank space (a whole line of spaces)
    // 0x20 = Space, 0x0F = White on Black
    uint16 blank = 0x20 | (WHITE_ON_BLACK << 8);
    uint16* video_memory = (uint16*) VGA_ADDRESS;

    // 2. Move lines 1-24 UP to rows 0-23
    // We iterate from 0 to 24*80
    for (int i = 0; i < (VGA_ROWS - 1) * VGA_COLS; i++) {
        video_memory[i] = video_memory[i + VGA_COLS];
    }

    // 3. Clear the last line (Row 24)
    for (int i = (VGA_ROWS - 1) * VGA_COLS; i < VGA_ROWS * VGA_COLS; i++) {
        video_memory[i] = blank;
    }

    // 4. Move cursor back to the last line
    cursor_y = VGA_ROWS - 1;
}

// Helper: Print a single character
void print_char(char c) {
    uint16* video_memory = (uint16*) VGA_ADDRESS;

    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        // Calculate offset: (y * 80) + x
        int offset = (cursor_y * VGA_COLS) + cursor_x;

        // Write [Attribute][ASCII]
        video_memory[offset] = (c) | (WHITE_ON_BLACK << 8);
        cursor_x++;
    }

    // Check if we hit the edge of the screen
    if (cursor_x >= VGA_COLS) {
        cursor_x = 0;
        cursor_y++;
    }

    // Check if we need to scroll
    if (cursor_y >= VGA_ROWS) {
        scroll();
    }
}

// --- PUBLIC FUNCTIONS ---

// 1. Clear Screen
void clear_screen() {
    uint16 blank = 0x20 | (WHITE_ON_BLACK << 8);
    uint16* video_memory = (uint16*) VGA_ADDRESS;

    for (int i = 0; i < VGA_COLS * VGA_ROWS; i++) {
        video_memory[i] = blank;
    }
    cursor_x = 0;
    cursor_y = 0;
}

// 2. Print String
void print(const char* str) {
    int i = 0;
    while (str[i] != 0) {
        print_char(str[i]);
        i++;
    }
}

// 3. Print Hex (Crucial for debugging Pointers/Values)
// Example output: "0x0001F00D"
void print_hex(unsigned int n) {
    print("0x");

    char hex_chars[] = "0123456789ABCDEF";

    // We print 8 hex digits (32-bit integer)
    // We loop from top nibble (28 bits shift) down to 0
    for (int i = 28; i >= 0; i -= 4) {
        // Extract 4 bits
        int nibble = (n >> i) & 0xF;
        print_char(hex_chars[nibble]);
    }
}