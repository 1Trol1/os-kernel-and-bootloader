#include "kheap.h"
#include "vga.h"

static uint64_t heap_start = 0;
static uint64_t heap_end   = 0;
static uint64_t heap_curr  = 0;

void kheap_init(uint64_t start, uint64_t size) {
    heap_start = start;
    heap_end   = start + size;
    heap_curr  = start;

    vga_print("kheap: init at ");
    vga_print_hex64(start);
    vga_print(" size=");
    vga_print_hex64(size);
    vga_print("\n");
}

void* kmalloc(uint64_t size) {
    // wyrównanie do 16 bajtów
    size = (size + 15) & ~15ULL;

    if (heap_curr + size > heap_end) {
        vga_print("kheap: OUT OF MEMORY\n");
        return 0;
    }

    void* ptr = (void*)heap_curr;
    heap_curr += size;

    return ptr;
}

void kfree(void* ptr) {
    // na razie nic — bump allocator nie wspiera free
    (void)ptr;
}

