#include <stdint.h>
#include "nvme.h"
#include "nvme_regs.h"
#include "pci.h"
#include "vga.h"

#define NVME_CC_EN     (1u << 0)
#define NVME_CSTS_RDY  (1u << 0)

static volatile struct nvme_regs* nvme = 0;

// admin queue
#define NVME_ADMIN_Q_DEPTH 32
static struct nvme_cmd admin_sq[NVME_ADMIN_Q_DEPTH] __attribute__((aligned(4096)));
static struct nvme_cqe admin_cq[NVME_ADMIN_Q_DEPTH] __attribute__((aligned(4096)));
static uint16_t admin_sq_tail = 0;
static uint16_t admin_cq_head = 0;
static uint8_t  admin_cq_phase = 1;

// I/O queue (queue id = 1)
#define NVME_IO_Q_DEPTH 64
static struct nvme_cmd io_sq[NVME_IO_Q_DEPTH] __attribute__((aligned(4096)));
static struct nvme_cqe io_cq[NVME_IO_Q_DEPTH] __attribute__((aligned(4096)));
static uint16_t io_sq_tail = 0;
static uint16_t io_cq_head = 0;
static uint8_t  io_cq_phase = 1;

static uint32_t g_block_size = 512;
uint32_t nvme_get_block_size(void) { return g_block_size; }

static uint32_t nvme_dstrd = 0; // doorbell stride (2^dstrd * 4 bytes)

// doorbell helpers
static volatile uint32_t* nvme_sq_doorbell(uint16_t qid) {
    uintptr_t base = (uintptr_t)nvme + 0x1000;
    uintptr_t off  = (2u * qid) * (4u << nvme_dstrd);
    return (volatile uint32_t*)(base + off);
}

static volatile uint32_t* nvme_cq_doorbell(uint16_t qid) {
    uintptr_t base = (uintptr_t)nvme + 0x1000;
    uintptr_t off  = (2u * qid + 1u) * (4u << nvme_dstrd);
    return (volatile uint32_t*)(base + off);
}

int nvme_find(uint64_t ecam_base, struct nvme_device* out) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t dev = 0; dev < 32; dev++) {
            for (uint8_t func = 0; func < 8; func++) {

                uint16_t vendor = pci_read16(ecam_base, bus, dev, func, 0x00);
                if (vendor == 0xFFFF) continue;

                uint8_t class_code = pci_read8(ecam_base, bus, dev, func, 0x0B);
                uint8_t subclass   = pci_read8(ecam_base, bus, dev, func, 0x0A);
                uint8_t prog_if    = pci_read8(ecam_base, bus, dev, func, 0x09);

                if (class_code == 0x01 && subclass == 0x08 && prog_if == 0x02) {
                    vga_print("NVMe found at ");
                    vga_print_hex(bus);
                    vga_print(":");
                    vga_print_hex(dev);
                    vga_print(".");
                    vga_print_hex(func);
                    vga_print("\n");

                    uint32_t bar_low  = pci_read32(ecam_base, bus, dev, func, 0x10);
                    uint32_t bar_high = pci_read32(ecam_base, bus, dev, func, 0x14);

                    uint64_t bar = ((uint64_t)bar_high << 32) | bar_low;
                    bar &= ~0xFULL;

                    out->bar0 = bar;
                    out->bus  = bus;
                    out->dev  = dev;
                    out->func = func;

                    return 1;
                }
            }
        }
    }
    return 0;
}

static void nvme_wait_ready(int ready) {
    if (ready) {
        while ((nvme->csts & NVME_CSTS_RDY) == 0)
            __asm__ volatile("pause");
    } else {
        while ((nvme->csts & NVME_CSTS_RDY) != 0)
            __asm__ volatile("pause");
    }
}

static void nvme_parse_cap(void) {
    uint64_t cap = nvme->cap;
    uint8_t dstrd = (cap >> 32) & 0xF; // DSTRD bits
    nvme_dstrd = dstrd;

    vga_print("NVMe CAP: ");
    vga_print_hex64(cap);
    vga_print("\n");
}

static void nvme_setup_admin_queue(void) {
    uint32_t asqs = NVME_ADMIN_Q_DEPTH - 1;
    uint32_t acqs = NVME_ADMIN_Q_DEPTH - 1;
    nvme->aqa = ((uint64_t)acqs << 16) | (uint64_t)asqs;

    nvme->asq = (uint64_t)admin_sq;
    nvme->acq = (uint64_t)admin_cq;

    admin_sq_tail = 0;
    admin_cq_head = 0;
    admin_cq_phase = 1;
}

static void nvme_enable_controller(void) {
    uint32_t cc = nvme->cc;

    // MPS: 0 = 4K page (2^(12+0))
    // IOSQES: 6 -> 64B
    // IOCQES: 4 -> 16B
    cc &= ~((0xF << 7) | (0xF << 20) | (0xF << 16)); // wyczyść MPS, IOSQES, IOCQES
    cc |= (0 << 7);   // MPS = 0 (4K)
    cc |= (6 << 20);  // IOSQES = 6 (64B)
    cc |= (4 << 16);  // IOCQES = 4 (16B)

    nvme->cc = cc & ~NVME_CC_EN;
    nvme_wait_ready(0);

    nvme->cc = cc | NVME_CC_EN;
    nvme_wait_ready(1);

    vga_print("NVMe: controller enabled\n");
}

static struct nvme_cqe* nvme_poll_cq(struct nvme_cqe* cq,
                                     uint16_t* head,
                                     uint8_t* phase,
                                     uint16_t q_depth,
                                     uint16_t qid) {
    while (1) {
        struct nvme_cqe* cqe = &cq[*head];
        uint16_t status = cqe->status;
        uint8_t  p = status & 1;

        if (p != *phase) {
            __asm__ volatile("pause");
            continue;
        }

        uint16_t sc = (status >> 1) & 0x7FF; // status code + type
        if (sc != 0) {
            vga_print("NVMe: CQ error, status=");
            vga_print_hex(sc);
            vga_print("\n");
        }

        *head = (*head + 1) % q_depth;
        if (*head == 0) {
            *phase ^= 1;
        }

        *nvme_cq_doorbell(qid) = *head;
        return cqe;
    }
}

static void nvme_submit_sq(struct nvme_cmd* sq,
                           uint16_t* tail,
                           uint16_t q_depth,
                           uint16_t qid,
                           const struct nvme_cmd* cmd) {
    sq[*tail] = *cmd;
    *tail = (*tail + 1) % q_depth;
    *nvme_sq_doorbell(qid) = *tail;
}

// Identify buffers
static uint8_t identify_ctrl_buf[4096] __attribute__((aligned(4096)));
static uint8_t identify_ns_buf[4096]   __attribute__((aligned(4096)));

static int nvme_identify_controller(void) {
    struct nvme_cmd cmd = {0};
    cmd.opcode = 0x06;
    cmd.cid    = 1;
    cmd.nsid   = 0;
    cmd.prp1   = (uint64_t)identify_ctrl_buf;
    cmd.cdw10  = 1; // CNS = 1 -> Identify Controller

    nvme_submit_sq(admin_sq, &admin_sq_tail, NVME_ADMIN_Q_DEPTH, 0, &cmd);
    nvme_poll_cq(admin_cq, &admin_cq_head, &admin_cq_phase, NVME_ADMIN_Q_DEPTH, 0);

    vga_print("NVMe: Identify Controller OK\n");
    return 1;
}

static int nvme_identify_namespace(uint32_t nsid) {
    struct nvme_cmd cmd = {0};
    cmd.opcode = 0x06;
    cmd.cid    = 2;
    cmd.nsid   = nsid;
    cmd.prp1   = (uint64_t)identify_ns_buf;
    cmd.cdw10  = 0; // CNS = 0 -> Identify Namespace

    nvme_submit_sq(admin_sq, &admin_sq_tail, NVME_ADMIN_Q_DEPTH, 0, &cmd);
    nvme_poll_cq(admin_cq, &admin_cq_head, &admin_cq_phase, NVME_ADMIN_Q_DEPTH, 0);

    uint64_t nsze = *(uint64_t*)&identify_ns_buf[0x00];
    uint8_t  flbas = identify_ns_buf[0x28];
    uint8_t  lba_format = flbas & 0x0F;

    uint32_t lbaf = *(uint32_t*)&identify_ns_buf[0x30 + lba_format * 4];
    uint8_t  lbads = lbaf & 0xFF;

    g_block_size = 1u << lbads;

    vga_print("NVMe: NS size (blocks)=");
    vga_print_hex64(nsze);
    vga_print("\n");

    vga_print("NVMe: block size=");
    vga_print_hex(g_block_size);
    vga_print("\n");

    return 1;
}

static void nvme_setup_io_queue(void) {
    // Create I/O Completion Queue (queue id = 1)
    {
        struct nvme_cmd cmd = {0};
        cmd.opcode = 0x05; // Create I/O Completion Queue
        cmd.cid    = 3;
        cmd.prp1   = (uint64_t)io_cq;
        cmd.cdw10  = (1u << 16) | (NVME_IO_Q_DEPTH - 1); // QID=1, QSIZE=N-1
        cmd.cdw11  = (1u << 0); // PC=1 (physically contiguous), IEN=0

        nvme_submit_sq(admin_sq, &admin_sq_tail, NVME_ADMIN_Q_DEPTH, 0, &cmd);
        nvme_poll_cq(admin_cq, &admin_cq_head, &admin_cq_phase, NVME_ADMIN_Q_DEPTH, 0);
    }

    // Create I/O Submission Queue (queue id = 1)
    {
        struct nvme_cmd cmd = {0};
        cmd.opcode = 0x01; // Create I/O Submission Queue
        cmd.cid    = 4;
        cmd.prp1   = (uint64_t)io_sq;
        cmd.cdw10  = (1u << 16) | (NVME_IO_Q_DEPTH - 1); // QID=1, QSIZE=N-1
        cmd.cdw11  = (1u << 0) | (1u << 16); // PC=1, CQID=1

        nvme_submit_sq(admin_sq, &admin_sq_tail, NVME_ADMIN_Q_DEPTH, 0, &cmd);
        nvme_poll_cq(admin_cq, &admin_cq_head, &admin_cq_phase, NVME_ADMIN_Q_DEPTH, 0);
    }

    io_sq_tail = 0;
    io_cq_head = 0;
    io_cq_phase = 1;

    vga_print("NVMe: I/O queue created\n");
}

int nvme_init(struct nvme_device* dev) {
    nvme = (volatile struct nvme_regs*)(uintptr_t)dev->bar0;

    nvme_parse_cap();

    // disable
    nvme->cc &= ~NVME_CC_EN;
    nvme_wait_ready(0);

    nvme_setup_admin_queue();
    nvme_enable_controller();

    nvme_identify_controller();
    nvme_identify_namespace(1);

    nvme_setup_io_queue();

    return 1;
}

// Bufor do I/O – jedna strona 4K, wyrównana
static uint8_t nvme_io_buf[4096] __attribute__((aligned(4096)));

int nvme_read_blocks(uint32_t nsid,
                     uint64_t lba,
                     uint32_t count,
                     void* buffer,
                     uint32_t buffer_size) {
    uint32_t blk_size = g_block_size;
    uint64_t total_bytes = (uint64_t)count * blk_size;

    if (total_bytes > sizeof(nvme_io_buf)) {
        vga_print("NVMe: read too big for single-PRP buffer\n");
        return 0;
    }

    if (buffer_size < total_bytes) {
        vga_print("NVMe: destination buffer too small\n");
        return 0;
    }

    struct nvme_cmd cmd = {0};
    cmd.opcode = 0x02; // READ
    cmd.cid    = 10;
    cmd.nsid   = nsid;

    cmd.prp1 = (uint64_t)nvme_io_buf;
    cmd.prp2 = 0;

    cmd.cdw10 = (uint32_t)lba;
    cmd.cdw11 = (uint32_t)(lba >> 32);
    cmd.cdw12 = count - 1;

    nvme_submit_sq(io_sq, &io_sq_tail, NVME_IO_Q_DEPTH, 1, &cmd);
    nvme_poll_cq(io_cq, &io_cq_head, &io_cq_phase, NVME_IO_Q_DEPTH, 1);

    // kopiujemy do docelowego bufora
    uint8_t* src = nvme_io_buf;
    uint8_t* dst = (uint8_t*)buffer;
    for (uint64_t i = 0; i < total_bytes; i++) {
        dst[i] = src[i];
    }

    return 1;
}

