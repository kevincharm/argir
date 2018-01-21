#include <stddef.h>
#include <stdint.h>
#include <memory.h>

void *memset(void *s, int c, size_t n)
{
    uint8_t *mem = (uint8_t *)s;
    for (size_t i=0; i<n; i++) {
        *(mem+i) = (uint8_t)c;
    }
}
