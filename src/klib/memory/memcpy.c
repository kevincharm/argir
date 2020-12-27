#include <stddef.h>
#include <stdint.h>
#include <memory.h>

void *memcpy(void *restrict dst, const void *restrict src, size_t n)
{
    for (size_t i = 0; i < n; i++)
        *((uint8_t *)dst + i) = *((uint8_t *)src + i);
}
