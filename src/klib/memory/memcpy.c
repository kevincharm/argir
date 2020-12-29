#include <stddef.h>
#include <stdint.h>
#include <memory.h>

void *memcpy(void *restrict dst, const void *restrict src, size_t n)
{
    void *dest = dst;
    __asm__ volatile("rep movsb"
                     : "+D"(dest)
                     : "c"(n), "S"(src)
                     : "cc", "memory");
    return dst;
}
