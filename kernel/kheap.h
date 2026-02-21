#pragma once
#include <stdint.h>

void  kheap_init(uint64_t start, uint64_t size);
void* kmalloc(uint64_t size);
void  kfree(void* ptr); // na razie no-op

