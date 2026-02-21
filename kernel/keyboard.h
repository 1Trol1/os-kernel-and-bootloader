#pragma once
#include <stdint.h>

#define KBD_BUFFER_SIZE 256

// Bufor znaków wpisywanych przez użytkownika
extern char kbd_buffer[KBD_BUFFER_SIZE];

// Aktualna długość bufora (volatile, bo modyfikowane w IRQ)
extern volatile int kbd_buf_len;

// Flaga: 1 gdy linia zakończona ENTER jest gotowa do odczytu
extern volatile int kbd_line_ready;

// Inicjalizacja drivera klawiatury
void keyboard_init(void);

// Handler IRQ1 (wywoływany z Twojego IRQ dispatcher)
void keyboard_on_irq(void);

// Blokujące czytanie linii (czeka aż user naciśnie ENTER)
int keyboard_readline(char* out, int max_len);

