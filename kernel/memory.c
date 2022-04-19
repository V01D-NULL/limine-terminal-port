#include <memory.h>

size_t strlen(const char *s)
{
    size_t ret = 0;
    while (*s++)
        ret++;

    return ret;
}

void memcpy64(uint64_t *dst, const uint64_t *src, uint64_t n)
{
    __asm__(
        "rep movsq"
        :
        : "S"(src), "D"(dst), "c"(n)
        : "memory"
    );
}