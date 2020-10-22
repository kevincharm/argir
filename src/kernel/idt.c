#include <stdio.h>
#include <kernel/cpu.h>

static inline void lidt(uint32_t base, uint16_t limit)
{
    struct idtr IDTR = { .limit = limit, .base = base };

    __asm__ volatile("lidt %0" : : "m"(IDTR));
}

void idt_entry_set(size_t index, uint32_t base, uint8_t selector, uint8_t flags)
{
    struct idt_entry *entry = idt + index;
    entry->base_lo = (base >> 0u) & 0xffff;
    entry->base_hi = (base >> 16u) & 0xffff;
    entry->selector = selector;
    entry->flags = flags;
    entry->unused = 0;
}

void idt_load()
{
    lidt((uint32_t)&idt, (sizeof(struct idt_entry) * IDT_ENTRIES_COUNT) - 1);
    printf("Loaded interrupt descriptor table.\n");
}
