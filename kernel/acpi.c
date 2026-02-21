#include "acpi.h"
#include <stddef.h>
#include <stdint.h>
#include "vga.h"

static int checksum(const uint8_t* ptr, size_t len) {
    uint8_t sum = 0;
    for (size_t i = 0; i < len; i++)
        sum += ptr[i];
    return sum == 0;
}

const struct rsdp* acpi_find_rsdp(void) {
    // BIOS/UEFI area 0xE0000–0xFFFFF
    for (uint64_t addr = 0xE0000; addr < 0x100000; addr += 16) {
        struct rsdp* r = (struct rsdp*)addr;
        if (!__builtin_memcmp(r->signature, "RSD PTR ", 8)) {
            if (checksum((const uint8_t*)r, 20)) {
                if (r->revision >= 2 && checksum((const uint8_t*)r, r->length))
                    return r;
                return r;
            }
        }
    }

    return NULL;
}

const struct xsdt* acpi_get_xsdt(const struct rsdp* rsdp) {
    if (!rsdp) return NULL;

    if (rsdp->revision >= 2 && rsdp->xsdt_addr != 0) {
        return (const struct xsdt*)(uintptr_t)rsdp->xsdt_addr;
    }

    if (rsdp->rsdt_addr != 0) {
        return (const struct xsdt*)(uintptr_t)rsdp->rsdt_addr;
    }

    return NULL;
}

const struct acpi_sdt_header* acpi_find_table(const struct xsdt* xsdt, const char sig[4]) {
    if (!xsdt) return NULL;

    int entries = (xsdt->header.length - sizeof(xsdt->header)) / 8;

    for (int i = 0; i < entries; i++) {
        struct acpi_sdt_header* hdr =
            (struct acpi_sdt_header*)(uintptr_t)xsdt->entries[i];

        if (!__builtin_memcmp(hdr->signature, sig, 4)) {
            if (!checksum((const uint8_t*)hdr, hdr->length)) {
                vga_print("ACPI: bad checksum for table ");
                vga_write(hdr->signature, 4);
                vga_print("\n");
                continue;
            }
            return hdr;
        }
    }

    return NULL;
}

static struct madt_info madt_global = {0};

const struct madt_info* acpi_parse_madt(const struct acpi_sdt_header* hdr) {
    if (!hdr) return NULL;

    struct madt* m = (struct madt*)hdr;

    if (!checksum((const uint8_t*)m, m->header.length)) {
        vga_print("ACPI: MADT checksum failed\n");
        return NULL;
    }

    madt_global.lapic_addr      = m->lapic_addr;
    madt_global.ioapic_addr     = 0;
    madt_global.ioapic_gsi_base = 0;
    madt_global.override_count  = 0;

    uint8_t* ptr = m->entries;
    uint8_t* end = (uint8_t*)m + m->header.length;

    while (ptr < end) {
        uint8_t type = ptr[0];
        uint8_t len  = ptr[1];
        if (len < 2) break;

        if (type == MADT_IOAPIC) {
            struct madt_ioapic* io = (struct madt_ioapic*)ptr;
            madt_global.ioapic_addr     = io->ioapic_addr;
            madt_global.ioapic_gsi_base = io->gsi_base;
        } else if (type == MADT_INT_OVERRIDE) {
            struct madt_int_override* o = (struct madt_int_override*)ptr;
            if (madt_global.override_count < 16) {
                struct madt_override_info* dst =
                    &madt_global.overrides[madt_global.override_count++];
                dst->irq_source = o->irq_source;
                dst->gsi        = o->gsi;
                dst->flags      = o->flags;
            }
        }

        ptr += len;
    }

    return &madt_global;
}

const struct mcfg_entry* acpi_parse_mcfg(const struct acpi_sdt_header* hdr, int* count) {
    if (!hdr) return NULL;

    struct mcfg* m = (struct mcfg*)hdr;

    if (!checksum((const uint8_t*)m, m->header.length)) {
        vga_print("ACPI: MCFG checksum failed\n");
        return NULL;
    }

    int total = (m->header.length - sizeof(struct mcfg)) / sizeof(struct mcfg_entry);
    if (count) *count = total;

    return (const struct mcfg_entry*)m->entries;
}

