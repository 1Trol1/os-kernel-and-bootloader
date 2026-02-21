#include "tss.h"
#include "string.h"

static struct tss_entry tss;

struct tss_entry* tss_get(void) {
    return &tss;
}

void tss_init(void) {
    memset(&tss, 0, sizeof(tss));

    // ============================
    //  Stos IST1 dla double fault
    // ============================
    static uint8_t f_dstack[4096] __attribute__((aligned(16)));
    tss.ist1 = (uint64_t)(f_dstack + sizeof(f_dstack));

    // ============================
    //  Stos RSP0 (kernel stack)
    // ============================
    static uint8_t kernel_stack[8192] __attribute__((aligned(16)));
    tss.rsp0 = (uint64_t)(kernel_stack + sizeof(kernel_stack));

    // ============================
    //  I/O map — wyłączona
    // ============================
    tss.iomap_base = sizeof(struct tss_entry);
}

