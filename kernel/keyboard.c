#include <stdint.h>
#include "keyboard.h"
#include "vga.h"
#include "lapic.h"

#define KBD_DATA_PORT 0x60

char kbd_buffer[KBD_BUFFER_SIZE];
volatile int kbd_line_ready = 0;
volatile int kbd_buf_len = 0;

static int shift_pressed = 0;
static int ctrl_pressed  = 0;
static int alt_pressed   = 0;
static int caps_lock     = 0;

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// scancode set 1 – mapowanie bez shift
static const char scancode_normal[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',  0, '\\',
    'z','x','c','v','b','n','m',',','.','/',  0,   0,   0,   ' ',
    // reszta zer
};

// scancode set 1 – mapowanie z shift
static const char scancode_shift[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,
    'A','S','D','F','G','H','J','K','L',':','"','~',  0, '|',
    'Z','X','C','V','B','N','M','<','>','?',  0,   0,   0,   ' ',
    // reszta zer
};

void keyboard_init(void) {
    kbd_buf_len = 0;
    kbd_line_ready = 0;
    shift_pressed = ctrl_pressed = alt_pressed = caps_lock = 0;
}

// prosta funkcja do obsługi backspace na VGA
static void vga_backspace(void) {
    vga_putc('\b');
    vga_putc(' ');
    vga_putc('\b');
}

static char translate_scancode(uint8_t sc) {
    char c;

    if (shift_pressed) {
        c = scancode_shift[sc];
    } else {
        c = scancode_normal[sc];
    }

    // litery a-z: uwzględnij CAPS LOCK
    if (c >= 'a' && c <= 'z') {
        if (caps_lock ^ shift_pressed) {
            c = c - 'a' + 'A';
        }
    } else if (c >= 'A' && c <= 'Z') {
        if (caps_lock ^ shift_pressed) {
            c = c - 'A' + 'a';
        }
    }

    return c;
}

void keyboard_on_irq(void) {
    uint8_t sc = inb(KBD_DATA_PORT);

    // break code (key release)
    if (sc & 0x80) {
        uint8_t make = sc & 0x7F;
        switch (make) {
            case 0x2A: // left shift
            case 0x36: // right shift
                shift_pressed = 0;
                break;
            case 0x1D: // ctrl
                ctrl_pressed = 0;
                break;
            case 0x38: // alt
                alt_pressed = 0;
                break;
            default:
                break;
        }
        lapic_eoi();
        return;
    }

    // make code (key press)
    switch (sc) {
        case 0x2A: // left shift
        case 0x36: // right shift
            shift_pressed = 1;
            lapic_eoi();
            return;
        case 0x1D: // ctrl
            ctrl_pressed = 1;
            lapic_eoi();
            return;
        case 0x38: // alt
            alt_pressed = 1;
            lapic_eoi();
            return;
        case 0x3A: // caps lock
            caps_lock ^= 1;
            lapic_eoi();
            return;
        default:
            break;
    }

    if (sc >= 128) {
        lapic_eoi();
        return;
    }

    char c = translate_scancode(sc);

    if (c == '\b') {
        if (kbd_buf_len > 0 && !kbd_line_ready) {
            kbd_buf_len--;
            vga_backspace();
        }
        lapic_eoi();
        return;
    }

    if (c == '\n') {
        if (!kbd_line_ready) {
            vga_putc('\n');
            if (kbd_buf_len < KBD_BUFFER_SIZE - 1) {
                kbd_buffer[kbd_buf_len++] = '\n';
            }
            kbd_buffer[kbd_buf_len] = 0;
            kbd_line_ready = 1;
        }
        lapic_eoi();
        return;
    }

    if (c) {
        if (!kbd_line_ready && kbd_buf_len < KBD_BUFFER_SIZE - 1) {
            kbd_buffer[kbd_buf_len++] = c;
            vga_putc(c);
        }
    }

    lapic_eoi();
}

// proste, blokujące czytanie linii
int keyboard_readline(char* out, int max_len) {
    // czekaj aż będzie gotowa linia
    while (!kbd_line_ready) {
        __asm__ volatile ("hlt");
    }

    int n = (kbd_buf_len < max_len - 1) ? kbd_buf_len : (max_len - 1);
    for (int i = 0; i < n; i++) {
        out[i] = kbd_buffer[i];
    }
    out[n] = 0;

    // zresetuj stan
    kbd_buf_len = 0;
    kbd_line_ready = 0;

    return n;
}

