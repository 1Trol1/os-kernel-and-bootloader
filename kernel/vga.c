// vga.c
#include "vga.h"

#define VGA_MEMORY 0xB8000
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

static uint16_t* const vga = (uint16_t*)VGA_MEMORY;
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;

static uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | (uint16_t)color << 8;
}

static void vga_scroll(void) {
    // przesuwamy linie o 1 w górę
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga[(y - 1) * VGA_WIDTH + x] = vga[y * VGA_WIDTH + x];
        }
    }

    // ostatnia linia = pusta
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', 0x07);
    }

    cursor_y = VGA_HEIGHT - 1;
}

void vga_clear(void) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga[y * VGA_WIDTH + x] = vga_entry(' ', 0x07);
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}

void vga_putc(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= VGA_HEIGHT)
            vga_scroll();
        return;
    }

    if (c == '\r') {
        cursor_x = 0;
        return;
    }

    vga[cursor_y * VGA_WIDTH + cursor_x] = vga_entry(c, 0x0F);
    cursor_x++;

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= VGA_HEIGHT)
            vga_scroll();
    }
}

void vga_print(const char* s) {
    while (*s) {
        vga_putc(*s++);
    }
}

void vga_print_hex(uint64_t value) {
    const char* hex = "0123456789ABCDEF";
    vga_print("0x");
    for (int i = 60; i >= 0; i -= 4) {
        vga_putc(hex[(value >> i) & 0xF]);
    }
}

void vga_print_hex64(uint64_t value) {
    vga_print_hex(value);
}

void vga_write(const char* str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        vga_putc(str[i]);
    }
}


