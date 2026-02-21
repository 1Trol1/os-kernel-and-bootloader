#include "string.h"

void* memcpy(void* dst, const void* src, size_t n) {
    unsigned char* d = dst;
    const unsigned char* s = src;
    while (n--) *d++ = *s++;
    return dst;
}

void* memset(void* dst, int value, size_t n) {
    unsigned char* d = dst;
    while (n--) *d++ = (unsigned char)value;
    return dst;
}

size_t strlen(const char* s) {
    size_t len = 0;
    while (*s++) len++;
    return len;
}

int memcmp(const void* a, const void* b, size_t n) {
    const unsigned char* p1 = a;
    const unsigned char* p2 = b;
    while (n--) {
        if (*p1 != *p2)
            return (int)*p1 - (int)*p2;
        p1++;
        p2++;
    }
    return 0;
}

