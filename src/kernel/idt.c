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

    // Check loaded IDTR
    struct dtr loaded_idtr = {
        .limit = 0xffff,
        .base = 0xffffffffffffffffull,
    };
    sidt(&loaded_idtr);
    if (idtr.limit != loaded_idtr.limit || idtr.base != loaded_idtr.base) {
        printf("Failed to load interrupt descriptor table!\n");
        __asm__ volatile("hlt");
    }

    printf("Loaded interrupt descriptor table.\n");
}
