#pragma once
#include <stdint.h>

uint32_t pci_read32(uint64_t ecam_base, uint8_t bus, uint8_t dev, uint8_t func, uint16_t offset);
uint16_t pci_read16(uint64_t ecam_base, uint8_t bus, uint8_t dev, uint8_t func, uint16_t offset);
uint8_t  pci_read8 (uint64_t ecam_base, uint8_t bus, uint8_t dev, uint8_t func, uint16_t offset);

void pci_enumerate(uint64_t ecam_base);

