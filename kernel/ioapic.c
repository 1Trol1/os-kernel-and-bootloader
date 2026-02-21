#include <stdint.h>
#include "ioapic.h"

static volatile uint32_t* ioapic = 0;
static uint32_t ioapic_gsi_base = 0;

static inline uint32_t ioapic_read(uint8_t reg) {
    ioapic[0] = reg;
    return ioapic[4];
}

static inline void ioapic_write(uint8_t reg, uint32_t val) {
    ioapic[0] = reg;
    ioapic[4] = val;
}

void ioapic_init(uint64_t ioapic_addr, uint32_t gsi_base) {
    ioapic = (volatile uint32_t*)(uintptr_t)ioapic_addr;
    ioapic_gsi_base = gsi_base;

    uint32_t max = (ioapic_read(1) >> 16) & 0xFF;

    // mask all IRQs
    for (uint32_t i = 0; i <= max; i++) {
        uint8_t reg = 0x10 + i * 2;
        ioapic_write(reg, 1 << 16); // mask
        ioapic_write(reg + 1, 0);   // dest = 0
    }
}

void ioapic_set_gsi(uint32_t gsi, uint8_t vector, uint8_t apic_id, uint16_t flags) {
    if (gsi < ioapic_gsi_base)
        return;

    uint32_t index = gsi - ioapic_gsi_base;
    uint8_t reg_low  = 0x10 + index * 2;
    uint8_t reg_high = reg_low + 1;

    uint32_t low  = vector;                 // fixed delivery
    uint32_t high = ((uint32_t)apic_id) << 24;

    // ACPI flags → IOAPIC:
    // bits 0–1: polarity, bits 2–3: trigger
    // 0 = conform, 1 = active high/edge, 3 = active low/level

    uint16_t pol = flags & 0x3;
    uint16_t trg = (flags >> 2) & 0x3;

    // polarity
    if (pol == 3)      // active low
        low |= (1 << 13);
    else               // conform / high
        low &= ~(1 << 13);

    // trigger mode
    if (trg == 3)      // level
        low |= (1 << 15);
    else               // conform / edge
        low &= ~(1 << 15);

    // na koniec: unmask
    low &= ~(1 << 16);

    ioapic_write(reg_high, high);
    ioapic_write(reg_low,  low);
}

void ioapic_mask(uint32_t gsi) {
    if (gsi < ioapic_gsi_base)
        return;

    uint32_t index = gsi - ioapic_gsi_base;
    uint8_t reg_low = 0x10 + index * 2;

    uint32_t low = ioapic_read(reg_low);
    low |= (1 << 16); // bit 16 = mask
    ioapic_write(reg_low, low);
}

void ioapic_unmask(uint32_t gsi) {
    if (gsi < ioapic_gsi_base)
        return;

    uint32_t index = gsi - ioapic_gsi_base;
    uint8_t reg_low = 0x10 + index * 2;

    uint32_t low = ioapic_read(reg_low);
    low &= ~(1 << 16); // bit 16 = mask
    ioapic_write(reg_low, low);
}

