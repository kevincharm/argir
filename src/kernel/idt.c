#include <stdio.h>
#include <kernel/cpu.h>

static inline void lidt(struct dtr *idtr)
{
    __asm__ volatile("lidt %0" : : "m"(idtr->limit));
}

static inline void sidt(struct dtr *idtr)
{
    __asm__ volatile("sidt %0" : "=m"(idtr->limit));
}

void set_interrupt_desc(size_t index, uint64_t base)
{
    struct gate_desc *entry = &idt[index];
    entry->offset_lo = (base >> 0u) & 0xffff;
    entry->offset_hi = (base >> 16u) & 0xffffffffffffll;
    entry->selector = IDT_CODE_SEGMENT;
    entry->ist = 0; // Legacy stack-switching mechanism
    entry->reserved_1 = 0;
    entry->type = SSDT_INTERRUPT_GATE;
    entry->zero = 0;
    entry->dpl = 0;
    entry->p = 1;
    entry->reserved_2 = 0;
}

void idt_init()
{
    struct dtr idtr = {
        .limit = ((sizeof(struct gate_desc)) * IDT_ENTRIES_COUNT - 1) & 0xffff,
        .base = idt,
    };
    lidt(&idtr);

    // Check
    sidt(&idtr);
    struct gate_desc *entries = (struct gate_desc *)idtr.base;
    printf("v=%u, base=%u, limit=%u\n", idt, idtr.base, idtr.limit);
    for (size_t i = 0; i < IDT_ENTRIES_COUNT; i++) {
        struct gate_desc *entry = &entries[i];
        // uint64_t isr_addr = (entry->offset_lo) &
        //                     ((entry->offset_hi << 16u) & 0xffffffffffff0000ll);
        if (i == 33 || i == 255) {
            printf("%u, ", entry->offset_lo & 0xffff);
        }
    }

    printf("Loaded interrupt descriptor table.\n");
}
