#include <stdint.h>
#include "irq.h"
#include "acpi.h"
#include "apic.h"
#include "ioapic.h"
#include "pic.h"
#include "vga.h"

// ------------------------------------------------------------
// Pomocnicza funkcja: znajdź GSI dla danego IRQ z MADT override
// ------------------------------------------------------------
static uint32_t madt_resolve_gsi(const struct madt_info* madt,
                                 uint8_t irq,
                                 uint16_t* out_flags)
{
    for (int i = 0; i < madt->override_count; i++) {
        if (madt->overrides[i].irq_source == irq) {
            if (out_flags) *out_flags = madt->overrides[i].flags;
            return madt->overrides[i].gsi;
        }
    }

    // brak override → klasyczne ISA: GSI = base + IRQ
    if (out_flags) *out_flags = 0;
    return madt->ioapic_gsi_base + irq;
}

// ------------------------------------------------------------
// Inicjalizacja systemu przerwań
// ------------------------------------------------------------
void irq_init(void) {
    const struct rsdp* rsdp = acpi_find_rsdp();
    if (!rsdp) {
        vga_print("ACPI: RSDP not found\n");
        return;
    }

    const struct xsdt* xsdt = acpi_get_xsdt(rsdp);
    if (!xsdt) {
        vga_print("ACPI: XSDT/RSDT not found\n");
        return;
    }

    const struct acpi_sdt_header* madt_hdr = acpi_find_table(xsdt, "APIC");
    if (!madt_hdr) {
        vga_print("ACPI: MADT not found\n");
        return;
    }

    const struct madt_info* madt = acpi_parse_madt(madt_hdr);
    if (!madt) {
        vga_print("ACPI: MADT parse failed\n");
        return;
    }

    // Wyłącz stary PIC
    pic_disable();

    // LAPIC
    lapic_init(madt->lapic_addr);

    // IOAPIC
    ioapic_init(madt->ioapic_addr, madt->ioapic_gsi_base);

    // --------------------------------------------------------
    // IRQ0 (timer) i IRQ1 (klawiatura) z obsługą override
    // --------------------------------------------------------
    uint16_t flags_timer, flags_kbd;

    uint32_t gsi_timer = madt_resolve_gsi(madt, 0, &flags_timer); // IRQ0
    uint32_t gsi_kbd   = madt_resolve_gsi(madt, 1, &flags_kbd);   // IRQ1

    // --------------------------------------------------------
    // Mapowanie GSI → LAPIC vector z uwzględnieniem flags
    // --------------------------------------------------------
    ioapic_set_gsi(gsi_timer, 32, 0, flags_timer);
    ioapic_set_gsi(gsi_kbd,   33, 0, flags_kbd);

    // Odmaskowanie
    ioapic_unmask(gsi_timer);
    ioapic_unmask(gsi_kbd);

    vga_print("IRQ: LAPIC + IOAPIC + MADT overrides initialized\n");
}

// ------------------------------------------------------------
// Maskowanie / odmaskowanie IRQ
// ------------------------------------------------------------
void irq_mask(int irq) {
    ioapic_mask(irq);
}

void irq_unmask(int irq) {
    ioapic_unmask(irq);
}

