// vga.h
#pragma once
#include <stdint.h>
#include <stddef.h>

void vga_clear(void);
void vga_putc(char c);
void vga_print(const char* s);
void vga_print_hex(uint64_t value);
void vga_print_hex64(uint64_t value);
void vga_write(const char* str, size_t len);

