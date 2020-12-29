#include <stddef.h>
#include "kernel/vmem.h"
#include "kernel/paging.h"
#include "kernel/colours.h"

// Base address of available space
uint64_t linear_base = 0;

/**
 * Allocate `n` bytes of virtual memory.
 * Physical memory and paging must be initialised already!
 */
void *vmem_alloc(size_t n)
{
    uint64_t base = linear_base;
    uint64_t limit = linear_base + n;
    if (limit >= linear_limit) {
        // OOM
        printf(BG_ROSSO("OOM") "\n");
        __asm__ volatile("mov $0xdeadbeef, %rax\n\t"
                         "1: jmp 1b");
    }

    linear_base = limit;
    return base;
}

void vmem_init()
{
    // TODO
}
