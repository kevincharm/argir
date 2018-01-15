#ifndef __ARGIR__IRQ_H
#define __ARGIR__IRQ_H

#include <stdint.h>

struct idt_entry {
    uint16_t base_lo;
    uint16_t selector;
    uint8_t unused; // always zero
    uint8_t flags;
    uint16_t base_hi;
};

struct idtr {
    uint16_t limit; // len of IDT in bytes - 1
    void *base;
}__attribute__((packed));

static inline void lidt(void *base, uint16_t limit)
{
    struct idtr IDTR = { limit, base };

    __asm__ volatile (
        "lidt %0"
        :
        : "m"(IDTR)
    );
}

void irq_init();

#endif /* __ARGIR__IRQ_H */
