#pragma once
#include <stdint.h>

void ioapic_init(uint64_t ioapic_addr, uint32_t gsi_base);
void ioapic_set_gsi(uint32_t gsi, uint8_t vector, uint8_t apic_id, uint16_t flags);


// maskowanie / odmaskowanie pojedynczego GSI
void ioapic_mask(uint32_t gsi);
void ioapic_unmask(uint32_t gsi);

