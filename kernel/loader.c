#include "loader.h"
#include "fat32.h"
#include "kheap.h"
#include "vga.h"

typedef void (*entry_fn)(void);

int loader_run_raw(const char* path) {
    struct fat32_file f;

    if (!fat32_open(path, &f)) {
        vga_print("Loader: file not found\n");
        return 0;
    }

    void* buf = kmalloc(f.size);
    if (!buf) {
        vga_print("Loader: kmalloc failed\n");
        return 0;
    }

    if (!fat32_read_all(&f, buf, f.size)) {
        vga_print("Loader: read failed\n");
        return 0;
    }

    vga_print("Loader: jumping to raw code at ");
    vga_print_hex64((uint64_t)buf);
    vga_print("\n");

    entry_fn entry = (entry_fn)buf;
    entry(); // skok do kodu

    return 1;
}

