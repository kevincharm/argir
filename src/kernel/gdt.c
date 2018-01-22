#include <stdio.h>
#include <kernel/cpu.h>

extern void gdt_rst(void);
static inline void lgdt(uint32_t base, uint16_t limit)
{
    struct gdtr GDTR = {
        .limit = limit,
        .base = base
    };

    __asm__ volatile (
        "lgdt %0"
        :
        : "m"(GDTR)
    );

    gdt_rst();
}

void gdt_entry_set(size_t index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    struct gdt_entry *entry = gdt+index;
    entry->base_lo = base & 0xffff;
    entry->base_mid = (base >> 16u) & 0xff;
    entry->base_hi = (base >> 24u) & 0xff;
    entry->limit_lo = limit & 0xffff;
    entry->gran = ((limit >> 16u) & 0x0f) | (gran & 0xf0);
    entry->access = access;
}

void gdt_load()
{
    lgdt((uint32_t)&gdt, (sizeof(struct gdt_entry) * GDT_ENTRIES_COUNT) - 1);
}

void gdt_init()
{
    gdt_entry_set(0, 0, 0, 0, 0);                   // null segment
    gdt_entry_set(1, 0, 0xffffffff, 0x9a, 0xcf);    // kernel code
    gdt_entry_set(2, 0, 0xffffffff, 0x92, 0xcf);    // kernel data

    gdt_load();
    printf("Loaded global descriptor table.\n");
}
