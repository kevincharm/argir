#ifndef __ARGIR__IRQ_H
#define __ARGIR__IRQ_H

#include <stdbool.h>
#include <stdint.h>

struct irq_frame {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // pushal
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

static inline void sti()
{
    __asm__("sti");
}

static inline void cli()
{
    __asm__("cli");
}

static inline bool irqs_enabled()
{
    uint32_t flags;
    __asm__ volatile (
        "pushf\n\t"
        "pop %0"
        : "=g"(flags)
    );
    return (flags >> 9u) & 0x1;
}

void irq_init();

#endif /* __ARGIR__IRQ_H */
