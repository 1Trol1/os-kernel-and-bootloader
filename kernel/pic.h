#pragma once
#include <stdint.h>

// Remap PIC to new interrupt vectors (usually 0x20–0x2F)
void pic_remap(int offset1, int offset2);

// Mask all IRQ lines (disable all PIC interrupts)
void pic_mask_all(void);

// Fully disable PIC (enter polling mode)
void pic_disable(void);

// Send End Of Interrupt to PIC (only used if PIC is active)
void pic_send_eoi(uint8_t irq);

// Unmask a specific IRQ line (enable one interrupt)
void pic_unmask_irq(uint8_t irq);

// Initialize PIC: remap + mask all
void pic_init(void);

