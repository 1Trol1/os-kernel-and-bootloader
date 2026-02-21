#pragma once
#include <stdint.h>

// Inicjalizacja systemu IRQ (ACPI → LAPIC → IOAPIC)
void irq_init(void);

// Główny dispatcher IRQ wywoływany przez isr.asm
void irq_handler_c(uint64_t* stack);

// Maskowanie / odmaskowanie IRQ (przez IOAPIC)
void irq_mask(int irq);
void irq_unmask(int irq);

