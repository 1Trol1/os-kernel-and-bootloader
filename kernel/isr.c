#include <stdint.h>
#include "vga.h"

// Funkcja do wysłania EOI do APIC (musisz mieć ją w apic.c)
extern void apic_eoi(void);

static void print_regs(uint64_t* s) {
    vga_print("\nRegisters:\n");

    vga_print("RAX="); vga_print_hex64(s[0]);
    vga_print(" RBX="); vga_print_hex64(s[1]);
    vga_print("\nRCX="); vga_print_hex64(s[2]);
    vga_print(" RDX="); vga_print_hex64(s[3]);
    vga_print("\nRBP="); vga_print_hex64(s[4]);
    vga_print(" RDI="); vga_print_hex64(s[5]);
    vga_print("\nRSI="); vga_print_hex64(s[6]);
    vga_print(" R8 ="); vga_print_hex64(s[7]);
    vga_print("\nR9 ="); vga_print_hex64(s[8]);
    vga_print(" R10="); vga_print_hex64(s[9]);
    vga_print("\nR11="); vga_print_hex64(s[10]);
    vga_print(" R12="); vga_print_hex64(s[11]);
    vga_print("\nR13="); vga_print_hex64(s[12]);
    vga_print(" R14="); vga_print_hex64(s[13]);
    vga_print("\nR15="); vga_print_hex64(s[14]);
    vga_print("\n");
}

void isr_handler_c(uint64_t* stack) {
    uint64_t int_no    = stack[15];
    uint64_t err_code  = stack[16];
    uint64_t rip       = stack[17];
    uint64_t cs        = stack[18];
    uint64_t rflags    = stack[19];
    uint64_t rsp       = stack[20];
    uint64_t ss        = stack[21];

    // IRQ (32–255)
    if (int_no >= 32) {
        // Timer, klawiatura, APIC timer itd.
        // Tu możesz dodać scheduler, obsługę klawiatury, itp.
        apic_eoi();
        return;
    }

    // ============================
    //  WYJĄTKI CPU (0–31)
    // ============================

    vga_print("\n=== CPU EXCEPTION ===\n");
    vga_print("Interrupt: ");
    vga_print_hex(int_no);
    vga_print("\nError code: ");
    vga_print_hex(err_code);
    vga_print("\nRIP: ");
    vga_print_hex64(rip);
    vga_print("\nRSP: ");
    vga_print_hex64(rsp);
    vga_print("\nRFLAGS: ");
    vga_print_hex64(rflags);
    vga_print("\nCS: ");
    vga_print_hex64(cs);
    vga_print("\nSS: ");
    vga_print_hex64(ss);
    vga_print("\n");

    // ============================
    //  PAGE FAULT (14)
    // ============================
    if (int_no == 14) {
        uint64_t cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));

        vga_print("Page Fault at address: ");
        vga_print_hex64(cr2);
        vga_print("\n");
    }

    print_regs(stack);

    vga_print("\nSystem halted.\n");

    for (;;) {
        __asm__("hlt");
    }
}

