#ifndef __ARGIR__IRQ_H
#define __ARGIR__IRQ_H

#include <stdbool.h>
#include <stdint.h>

#define IDT_SEL_KERNEL  (0x8)
#define IDT_FLAGS_BASE  (0xe)

struct interrupt_frame {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

struct idt_entry {
    uint16_t base_lo;
    uint16_t selector;
    uint8_t unused; // always zero
    uint8_t flags;
    uint16_t base_hi;
}__attribute__((packed));

struct idtr {
    uint16_t limit; // len of IDT in bytes - 1
    uint32_t base;
}__attribute__((packed));

static inline void lidt(uint32_t base, uint16_t limit)
{
    struct idtr IDTR = {
        .limit = limit,
        .base = base
    };

    __asm__ volatile (
        "lidt %0"
        :
        : "m"(IDTR)
    );
}

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

void idt_entry_set(struct idt_entry *entry, uint32_t base, uint8_t selector, uint8_t flags);
void irq_init();

#endif /* __ARGIR__IRQ_H */
