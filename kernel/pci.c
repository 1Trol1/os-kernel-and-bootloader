#include "pci.h"
#include "vga.h"

static inline uint64_t pci_ecam_addr(uint64_t base, uint8_t bus, uint8_t dev, uint8_t func, uint16_t offset) {
    return base
        + ((uint64_t)bus  << 20)
        + ((uint64_t)dev  << 15)
        + ((uint64_t)func << 12)
        + offset;
}

uint32_t pci_read32(uint64_t base, uint8_t bus, uint8_t dev, uint8_t func, uint16_t offset) {
    volatile uint32_t* ptr = (uint32_t*)(uintptr_t)pci_ecam_addr(base, bus, dev, func, offset);
    return *ptr;
}

uint16_t pci_read16(uint64_t base, uint8_t bus, uint8_t dev, uint8_t func, uint16_t offset) {
    volatile uint16_t* ptr = (uint16_t*)(uintptr_t)pci_ecam_addr(base, bus, dev, func, offset);
    return *ptr;
}

uint8_t pci_read8(uint64_t base, uint8_t bus, uint8_t dev, uint8_t func, uint16_t offset) {
    volatile uint8_t* ptr = (uint8_t*)(uintptr_t)pci_ecam_addr(base, bus, dev, func, offset);
    return *ptr;
}

void pci_enumerate(uint64_t ecam_base) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t dev = 0; dev < 32; dev++) {
            for (uint8_t func = 0; func < 8; func++) {

                uint16_t vendor = pci_read16(ecam_base, bus, dev, func, 0x00);
                if (vendor == 0xFFFF) continue;

                uint16_t device = pci_read16(ecam_base, bus, dev, func, 0x02);

                vga_print("PCIe: ");
                vga_print_hex(bus);
                vga_print(":");
                vga_print_hex(dev);
                vga_print(".");
                vga_print_hex(func);
                vga_print("  VID=");
                vga_print_hex(vendor);
                vga_print(" DID=");
                vga_print_hex(device);
                vga_print("\n");
            }
        }
    }
}

