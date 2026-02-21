#include "gdt.h"
#include "tss.h"
#include "string.h"

static struct gdt_entry gdt[7];
static struct gdt_ptr gdt_desc;

extern void gdt_flush(uint64_t);
extern void tss_flush(uint16_t);

static void gdt_set_entry(int i, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt[i].limit_low   = limit & 0xFFFF;
    gdt[i].base_low    = base & 0xFFFF;
    gdt[i].base_mid    = (base >> 16) & 0xFF;
    gdt[i].access      = access;
    gdt[i].granularity = (limit >> 16) & 0x0F;
    gdt[i].granularity |= gran & 0xF0;
    gdt[i].base_high   = (base >> 24) & 0xFF;
}

static void write_tss(int index, uint64_t tss_addr)
{
    uint64_t base = tss_addr;
    uint32_t limit = sizeof(struct tss_entry);

    gdt[index].limit_low = limit & 0xFFFF;
    gdt[index].base_low = base & 0xFFFF;
    gdt[index].base_mid = (base >> 16) & 0xFF;
    gdt[index].access = 0x89; // present, type=9 (64-bit TSS)
    gdt[index].granularity = (limit >> 16) & 0x0F;
    gdt[index].base_high = (base >> 24) & 0xFF;

    // TSS zajmuje 16 bajtów w GDT → drugi wpis
    uint32_t base_hi = (base >> 32);
    memcpy(&gdt[index + 1], &base_hi, 4);
}

void gdt_init(void)
{
    // ============================
    //  Inicjalizacja TSS
    // ============================
    tss_init();
    struct tss_entry* tss = tss_get();

    // ============================
    //  GDT entries
    // ============================
    gdt_set_entry(0, 0, 0, 0, 0);                // NULL
    gdt_set_entry(1, 0, 0, 0x9A, 0x20);          // Kernel Code
    gdt_set_entry(2, 0, 0, 0x92, 0x00);          // Kernel Data
    gdt_set_entry(3, 0, 0, 0xFA, 0x20);          // User Code
    gdt_set_entry(4, 0, 0, 0xF2, 0x00);          // User Data

    write_tss(5, (uint64_t)tss);

    gdt_desc.limit = sizeof(gdt) - 1;
    gdt_desc.base  = (uint64_t)&gdt;

    // ============================
    //  Załaduj GDT i TSS
    // ============================
    gdt_flush((uint64_t)&gdt_desc);
    tss_flush(5 << 3);
}

