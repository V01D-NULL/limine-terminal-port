#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

void memcpy64(uint64_t *dst, const uint64_t *src, uint64_t n);
size_t strlen(const char *s);

inline void outb(uint16_t port, uint8_t val)
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

#endif // MEMORY_H