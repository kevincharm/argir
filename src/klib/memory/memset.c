#include <stddef.h>
#include <stdint.h>
#include <memory.h>

void *memset(void *s, int c, size_t n)
{
    void *dest = s;
    __asm__ volatile("cld\n\t"
                     "rep stos %0, (%1)" ::"a"((uintptr_t)c),
                     "D"(dest), "c"(n / sizeof(uintptr_t))
                     : "memory");
    return s;
}
