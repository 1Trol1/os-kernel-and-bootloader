#include <stdint.h>
#include "vga.h"
#include "idt.h"
#include "pic.h"
#include "irq.h"
#include "pci.h"
#include "apic.h"
#include "ioapic.h"
#include "pci.h"
#include "nvme.h"
#include "gpt.h"
#include "fat32.h"
#include "kheap.h"
#include "acpi.h"   // zakładam, że masz taki nagłówek z rsdp/xsdt/madt/mcfg

void kernel_main(void) {
    vga_clear();
    vga_print("Kernel startuje...\n");

    // --- ACPI: RSDP, XSDT ---
    const struct rsdp* rsdp = acpi_find_rsdp();
    if (!rsdp) {
        vga_print("RSDP not found!\n");
        for(;;);
    }
    vga_print("RSDP OK\n");

    const struct xsdt* xsdt = acpi_get_xsdt(rsdp);
    if (!xsdt) {
        vga_print("XSDT not found!\n");
        for(;;);
    }
    vga_print("XSDT OK\n");

    // --- MADT: LAPIC + IOAPIC ---
    const struct acpi_sdt_header* madt_hdr = acpi_find_table(xsdt, "APIC");
    if (!madt_hdr) {
        vga_print("MADT not found\n");
        for(;;);
    }

    const struct madt_info* mi = acpi_parse_madt(madt_hdr);

    vga_print("MADT:\n");
    vga_print("  LAPIC addr: ");
    vga_print_hex(mi->lapic_addr);
    vga_print("\n  IOAPIC addr: ");
    vga_print_hex(mi->ioapic_addr);
    vga_print("\n  IOAPIC GSI base: ");
    vga_print_hex(mi->ioapic_gsi_base);
    vga_print("\n");

    lapic_init(mi->lapic_addr);
    ioapic_init(mi->ioapic_addr, mi->ioapic_gsi_base);

    // --- IDT / IRQ / PIC ---
    idt_init();
    vga_print("IDT OK\n");

    pic_remap(0x20, 0x28);
    pic_disable();
    vga_print("PIC OK\n");

    irq_init();
    vga_print("IRQ OK\n");

    // mapowanie GSI -> wektory (przykład)
    uint8_t bsp_apic_id = 0;
    ioapic_set_gsi(0, 32, bsp_apic_id, 0); // timer
    ioapic_set_gsi(1, 33, bsp_apic_id, 0); // klawiatura

    // --- heap kernela ---
    kheap_init(0x200000, 16 * 1024 * 1024); // 16 MB heap

    __asm__ volatile("sti");
    vga_print("Przerwania wlaczone\n");

    // --- MCFG / PCIe ECAM ---
    const struct acpi_sdt_header* mcfg_hdr = acpi_find_table(xsdt, "MCFG");
    if (!mcfg_hdr) {
        vga_print("MCFG not found\n");
        for(;;);
    }

    int mcfg_count = 0;
    const struct mcfg_entry* mcfg = acpi_parse_mcfg(mcfg_hdr, &mcfg_count);
    uint64_t ecam_base = mcfg[0].base_addr;

    vga_print("PCIe ECAM base: ");
    vga_print_hex64(ecam_base);
    vga_print("\n");

    pci_enumerate(ecam_base);

    // --- NVMe ---
    struct nvme_device nvme;
    if (!nvme_find(ecam_base, &nvme)) {
        vga_print("NVMe not found\n");
        for(;;);
    }

    vga_print("NVMe BAR0: ");
    vga_print_hex64(nvme.bar0);
    vga_print("\n");

    nvme_init(&nvme);

    uint32_t block_size = nvme_get_block_size(); // musisz mieć taką funkcję albo global
    
    vga_print("NVMe block size: ");
    vga_print_hex(block_size);
    vga_print("\n");

    // --- GPT ---
    struct gpt_header hdr;
    struct gpt_entry entries[128];

    if (!gpt_read_header(1, block_size, &hdr)) {
        vga_print("GPT header read failed\n");
        for(;;);
    }

    if (!gpt_read_entries(1, block_size, &hdr, entries)) {
        vga_print("GPT entries read failed\n");
        for(;;);
    }

    uint64_t part_start = 0, part_end = 0;
    if (!gpt_find_partition(&hdr, entries, &part_start, &part_end)) {
        vga_print("No usable partition found\n");
        for(;;);
    }

    vga_print("Partition start LBA: ");
    vga_print_hex64(part_start);
    vga_print("\n");

    vga_print("Partition end LBA: ");
    vga_print_hex64(part_end);
    vga_print("\n");

    // --- FAT32 ---
    if (!fat32_mount(part_start, block_size)) {
        vga_print("FAT32 mount failed\n");
        for(;;);
    }

    fat32_list_root();

    // --- test: odczyt pliku z root ---
    static uint8_t file_buf[4096];
    uint32_t file_size = 0;

    if (fat32_read_file_from_root("KERNEL.BIN", file_buf, sizeof(file_buf), &file_size)) {
        vga_print("Read KERNEL.BIN, size=");
        vga_print_hex64(file_size);
        vga_print("\n");
    } else {
        vga_print("Failed to read KERNEL.BIN\n");
    }

    // pętla główna
    for (;;) {
        __asm__ volatile("hlt");
    }
}

