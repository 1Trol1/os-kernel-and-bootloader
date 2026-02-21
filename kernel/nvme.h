#pragma once
#include <stdint.h>

struct nvme_device {
    uint64_t bar0;
    uint8_t  bus;
    uint8_t  dev;
    uint8_t  func;
};

// API
int      nvme_find(uint64_t ecam_base, struct nvme_device* out);
int      nvme_init(struct nvme_device* dev);
uint32_t nvme_get_block_size(void);

int nvme_read_blocks(uint32_t nsid,
                     uint64_t lba,
                     uint32_t count,
                     void* buffer,
                     uint32_t buffer_size);

// -----------------------------
//  NVMe command structures
// -----------------------------
struct nvme_cmd {
    uint8_t  opcode;
    uint8_t  flags;
    uint16_t cid;

    uint32_t nsid;

    uint64_t rsvd2;

    uint64_t mptr;
    uint64_t prp1;
    uint64_t prp2;

    uint32_t cdw10;
    uint32_t cdw11;
    uint32_t cdw12;
    uint32_t cdw13;
    uint32_t cdw14;
    uint32_t cdw15;
} __attribute__((packed));

struct nvme_cqe {
    uint32_t cdw0;
    uint32_t rsvd;
    uint16_t sq_head;
    uint16_t sq_id;
    uint16_t cid;
    uint16_t status; // bit0 = phase, reszta = status code
} __attribute__((packed));

