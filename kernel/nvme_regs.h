#pragma once
#include <stdint.h>

struct nvme_regs {
    uint64_t cap;      // 0x00
    uint32_t vs;       // 0x08
    uint32_t intms;    // 0x0C
    uint32_t intmc;    // 0x10
    uint32_t cc;       // 0x14
    uint32_t rsvd1;    // 0x18
    uint32_t csts;     // 0x1C
    uint32_t nssr;     // 0x20
    uint32_t aqa;      // 0x24  (UWAGA: 32-bit!)
    uint32_t rsvd2;    // 0x28  (padding)
    uint64_t asq;      // 0x30
    uint64_t acq;      // 0x38
    // doorbelle od 0x1000
} __attribute__((packed));

