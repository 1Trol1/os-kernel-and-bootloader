#pragma once
#include <stdint.h>

struct rsdp {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_addr;

    // ACPI 2.0+
    uint32_t length;
    uint64_t xsdt_addr;
    uint8_t extended_checksum;
    uint8_t reserved[3];
};

const struct rsdp* acpi_find_rsdp(void);

struct acpi_sdt_header {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
};

struct xsdt {
    struct acpi_sdt_header header;
    uint64_t entries[]; // zmienna liczba wpisów
};

struct xsdt;
struct acpi_sdt_header;

const struct xsdt* acpi_get_xsdt(const struct rsdp* rsdp);
const struct acpi_sdt_header* acpi_find_table(const struct xsdt* xsdt, const char sig[4]);
const struct mcfg_entry* acpi_parse_mcfg(const struct acpi_sdt_header* hdr, int* count);

struct madt_override_info {
    uint8_t irq_source;
    uint32_t gsi;
    uint16_t flags;
};

struct madt {
    struct acpi_sdt_header header;
    uint32_t lapic_addr;
    uint32_t flags;
    uint8_t entries[]; // zmienna liczba wpisów
};

enum madt_entry_type {
    MADT_LAPIC          = 0,
    MADT_IOAPIC         = 1,
    MADT_INT_OVERRIDE   = 2,
};

struct madt_lapic {
    uint8_t type;
    uint8_t length;
    uint8_t acpi_processor_id;
    uint8_t apic_id;
    uint32_t flags;
};

struct madt_ioapic {
    uint8_t type;
    uint8_t length;
    uint8_t ioapic_id;
    uint8_t reserved;
    uint32_t ioapic_addr;
    uint32_t gsi_base;
};

struct madt_int_override {
    uint8_t type;
    uint8_t length;
    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t gsi;
    uint16_t flags;
};


struct madt_info {
    uint32_t lapic_addr;
    uint32_t ioapic_addr;
    uint32_t ioapic_gsi_base;

    struct madt_override_info overrides[16];
    int override_count;
};

const struct madt_info* acpi_parse_madt(const struct acpi_sdt_header* hdr);

struct mcfg {
    struct acpi_sdt_header header;
    uint64_t reserved;
    uint8_t entries[];
};

struct mcfg_entry {
    uint64_t base_addr;
    uint16_t segment_group;
    uint8_t bus_start;
    uint8_t bus_end;
    uint32_t reserved;
};


