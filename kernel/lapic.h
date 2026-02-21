#pragma once
#include <stdint.h>

void lapic_init(uint64_t lapic_addr);
void lapic_eoi(void);

