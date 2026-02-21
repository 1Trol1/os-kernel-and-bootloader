#include "idt.h"

#define IDT_SIZE 256

static struct idt_entry idt[IDT_SIZE];
static struct idt_ptr idt_descriptor;

// wyjątki 0–31
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();   // double fault — wymaga IST
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();  // page fault
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

// IRQ
extern void irq0();
extern void irq1();

// APIC timer
extern void apic_timer();

static void idt_set_gate(int n, uint64_t handler, uint8_t ist) {
    idt[n].offset_low  = handler & 0xFFFF;
    idt[n].selector    = 0x08;
    idt[n].ist         = ist;       // <-- poprawka: obsługa IST
    idt[n].type_attr   = 0x8E;      // interrupt gate
    idt[n].offset_mid  = (handler >> 16) & 0xFFFF;
    idt[n].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[n].zero        = 0;
}

void idt_init(void) {
    // wyczyść IDT
    for (int i = 0; i < IDT_SIZE; i++) {
        idt[i].offset_low  = 0;
        idt[i].selector    = 0;
        idt[i].ist         = 0;
        idt[i].type_attr   = 0;
        idt[i].offset_mid  = 0;
        idt[i].offset_high = 0;
        idt[i].zero        = 0;
    }

    idt_descriptor.limit = sizeof(idt) - 1;
    idt_descriptor.base  = (uint64_t)&idt;

    // -------------------------
    //  WYJĄTKI CPU (0–31)
    // -------------------------
    idt_set_gate(0,  (uint64_t)isr0,  0);
    idt_set_gate(1,  (uint64_t)isr1,  0);
    idt_set_gate(2,  (uint64_t)isr2,  0);
    idt_set_gate(3,  (uint64_t)isr3,  0);
    idt_set_gate(4,  (uint64_t)isr4,  0);
    idt_set_gate(5,  (uint64_t)isr5,  0);
    idt_set_gate(6,  (uint64_t)isr6,  0);
    idt_set_gate(7,  (uint64_t)isr7,  0);

    // DOUBLE FAULT — MUSI mieć IST=1
    idt_set_gate(8,  (uint64_t)isr8,  1);

    idt_set_gate(9,  (uint64_t)isr9,  0);
    idt_set_gate(10, (uint64_t)isr10, 0);
    idt_set_gate(11, (uint64_t)isr11, 0);
    idt_set_gate(12, (uint64_t)isr12, 0);
    idt_set_gate(13, (uint64_t)isr13, 0);

    // PAGE FAULT — bardzo ważne
    idt_set_gate(14, (uint64_t)isr14, 0);

    idt_set_gate(15, (uint64_t)isr15, 0);
    idt_set_gate(16, (uint64_t)isr16, 0);
    idt_set_gate(17, (uint64_t)isr17, 0);
    idt_set_gate(18, (uint64_t)isr18, 0);
    idt_set_gate(19, (uint64_t)isr19, 0);
    idt_set_gate(20, (uint64_t)isr20, 0);
    idt_set_gate(21, (uint64_t)isr21, 0);
    idt_set_gate(22, (uint64_t)isr22, 0);
    idt_set_gate(23, (uint64_t)isr23, 0);
    idt_set_gate(24, (uint64_t)isr24, 0);
    idt_set_gate(25, (uint64_t)isr25, 0);
    idt_set_gate(26, (uint64_t)isr26, 0);
    idt_set_gate(27, (uint64_t)isr27, 0);
    idt_set_gate(28, (uint64_t)isr28, 0);
    idt_set_gate(29, (uint64_t)isr29, 0);
    idt_set_gate(30, (uint64_t)isr30, 0);
    idt_set_gate(31, (uint64_t)isr31, 0);

    // -------------------------
    //  IRQ (PIC/APIC)
    // -------------------------
    idt_set_gate(32, (uint64_t)irq0, 0);
    idt_set_gate(33, (uint64_t)irq1, 0);

    // APIC timer
    idt_set_gate(0x40, (uint64_t)apic_timer, 0);

    // załaduj IDT
    __asm__ volatile ("lidt %0" : : "m"(idt_descriptor));
}

