#include <stdint.h>
#include "lapic.h"

static volatile uint32_t* lapic = 0;

#define LAPIC_REG_ID            0x020
#define LAPIC_REG_TPR           0x080
#define LAPIC_REG_EOI           0x0B0
#define LAPIC_REG_SVR           0x0F0
#define LAPIC_REG_LVT_TIMER     0x320
#define LAPIC_REG_LVT_LINT0     0x350
#define LAPIC_REG_LVT_LINT1     0x360
#define LAPIC_REG_LVT_ERROR     0x370
#define LAPIC_REG_TIMER_INITCNT 0x380
#define LAPIC_REG_TIMER_CURRCNT 0x390
#define LAPIC_REG_TIMER_DIV     0x3E0

#define LAPIC_SVR_ENABLE        0x100
#define LAPIC_SVR_VECTOR        0xFF
#define LAPIC_TIMER_VECTOR      0x40
#define LAPIC_TIMER_PERIODIC    (1 << 17)

// minimalne I/O tylko dla tego pliku
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void lapic_write(uint32_t reg, uint32_t val) {
    lapic[reg / 4] = val;
    (void)lapic[reg / 4];
}

static inline uint32_t lapic_read(uint32_t reg) {
    return lapic[reg / 4];
}

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

static inline void wrmsr(uint32_t msr, uint64_t val) {
    uint32_t lo = (uint32_t)val;
    uint32_t hi = (uint32_t)(val >> 32);
    __asm__ volatile ("wrmsr" : : "c"(msr), "a"(lo), "d"(hi));
}

// czekamy ~10 ms używając PIT (porty 0x40–0x43)
static void pit_wait_10ms(void) {
    // channel 0, mode 2, binary, divisor ≈ 11932 → ~100 Hz (10 ms)
    outb(0x43, 0x34);
    outb(0x40, 11932 & 0xFF);
    outb(0x40, 11932 >> 8);

    uint16_t prev = 0xFFFF;
    for (;;) {
        // latch current count
        outb(0x43, 0x00);
        uint8_t lo = inb(0x40);
        uint8_t hi = inb(0x40);
        uint16_t val = ((uint16_t)hi << 8) | lo;

        if (val > prev) // licznik się „zawinął” → minęło ~10 ms
            break;

        prev = val;
    }
}

static uint32_t lapic_timer_freq = 0;

static void lapic_calibrate(void) {
    // divisor /16
    lapic_write(LAPIC_REG_TIMER_DIV, 0x3);

    // one-shot, vector 0x40
    lapic_write(LAPIC_REG_LVT_TIMER, LAPIC_TIMER_VECTOR);

    // duży initial count
    lapic_write(LAPIC_REG_TIMER_INITCNT, 0xFFFFFFFF);

    // czekamy ~10 ms
    pit_wait_10ms();

    uint32_t remaining = lapic_read(LAPIC_REG_TIMER_CURRCNT);
    uint32_t elapsed   = 0xFFFFFFFF - remaining;

    // 10 ms → *100 = na sekundę
    lapic_timer_freq = elapsed * 100;

    uint32_t ticks_per_ms = lapic_timer_freq / 1000;

    // przełącz na periodic
    lapic_write(LAPIC_REG_LVT_TIMER, LAPIC_TIMER_VECTOR | LAPIC_TIMER_PERIODIC);
    lapic_write(LAPIC_REG_TIMER_INITCNT, ticks_per_ms);
}

void lapic_init(uint64_t lapic_addr) {
    lapic = (volatile uint32_t*)(uintptr_t)lapic_addr;

    uint64_t apic_base = rdmsr(0x1B);
    apic_base |= (1ULL << 11);
    wrmsr(0x1B, apic_base);

    lapic_write(LAPIC_REG_SVR, LAPIC_SVR_ENABLE | LAPIC_SVR_VECTOR);

    lapic_write(LAPIC_REG_LVT_LINT0, 1 << 16);
    lapic_write(LAPIC_REG_LVT_LINT1, 1 << 16);
    lapic_write(LAPIC_REG_LVT_ERROR, 1 << 16);

    lapic_write(LAPIC_REG_TPR, 0);

    lapic_calibrate();

    lapic_write(LAPIC_REG_EOI, 0);
}

void lapic_eoi(void) {
    lapic_write(LAPIC_REG_EOI, 0);
}

