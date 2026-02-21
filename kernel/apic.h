#pragma once
#include <stdint.h>

// Inicjalizacja LAPIC — wymaga fizycznego adresu LAPIC z ACPI/MADT
void lapic_init(uint64_t lapic_addr);

// Wysłanie EOI do LAPIC
void lapic_eoi(void);

