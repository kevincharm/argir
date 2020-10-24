#include <stdio.h>
#include <kernel/cpu.h>

extern void gdt_rst(void);

static inline void lgdt(struct dtr *gdtr)
{
    __asm__ volatile("lgdt %0" : : "m"(gdtr->limit));
}

static inline void sgdt(struct dtr *gdtr)
{
    __asm__ volatile("sgdt %0" : "=m"(gdtr->limit));
}

void set_gen_segment_desc(size_t index, uint32_t base, uint32_t limit,
                          uint8_t type, uint8_t s, uint8_t dpl, uint8_t p,
                          uint8_t avl, uint8_t db, uint8_t g)
{
    struct gen_seg_desc *entry = gdt + index;
    entry->base_lo = base & 0xffff;
    entry->base_mid = (base >> 16u) & 0xff;
    entry->base_hi = (base >> 24u) & 0xff;
    entry->limit_lo = limit & 0xffff;
    entry->limit_hi_gran = ((g & 1) << 7u) | (((db & 1) << 6u)) |
                           ((avl & 1) << 5u) | ((limit >> 16u) & 0x0f);
    entry->access =
        ((p & 1) << 7u) | ((dpl & 2) << 5u) | ((s & 1) << 5u) | (type & 4);
}

void set_code_segment_desc(size_t index, uint32_t base, uint32_t limit,
                           uint8_t accessed, uint8_t readable,
                           uint8_t conforming, uint8_t dpl, uint8_t p,
                           uint8_t avl, uint8_t l, uint8_t d, uint8_t g)
{
    struct code_seg_desc *entry = gdt + index;
    entry->base_lo = base & 0xffff;
    entry->base_mid = (base >> 16u) & 0xff;
    entry->base_hi = (base >> 24u) & 0xff;
    entry->limit_lo = limit & 0xffff;
    entry->limit_hi = (limit >> 16u) & 0xffffffffffffll;
    entry->a = accessed & 1;
    entry->r = readable & 1;
    entry->c = conforming & 1;
    entry->one = 1;
    entry->another_one = 1;
    entry->dpl = dpl & 2;
    entry->p = p & 1;
    entry->avl = avl & 1;
    entry->l = l & 1;
    entry->d = d & 1;
    entry->g = g & 1;
}

void set_data_segment_desc(size_t index, uint32_t base, uint32_t limit,
                           uint8_t accessed, uint8_t writable,
                           uint8_t expand_down, uint8_t dpl, uint8_t p,
                           uint8_t avl, uint8_t l, uint8_t db, uint8_t g)
{
    struct data_seg_desc *entry = gdt + index;
    entry->base_lo = base & 0xffff;
    entry->base_mid = (base >> 16u) & 0xff;
    entry->base_hi = (base >> 24u) & 0xff;
    entry->limit_lo = limit & 0xffff;
    entry->limit_hi = (limit >> 16u) & 0xffffffffffffll;
    entry->a = accessed & 1;
    entry->w = writable & 1;
    entry->e = expand_down & 1;
    entry->one = 1;
    entry->zero = 0;
    entry->dpl = dpl & 2;
    entry->p = p & 1;
    entry->avl = avl & 1;
    entry->l = l & 1;
    entry->db = db & 1;
    entry->g = g & 1;
}

void gdt_init()
{
    set_gen_segment_desc(0, 0, 0, 0, 0, 0, 0, 0, 0, 0); // null segment
    set_code_segment_desc(1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0);
    set_data_segment_desc(2, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0);

    struct dtr gdtr = {
        .limit = (sizeof(struct gen_seg_desc) * GDT_ENTRIES_COUNT) - 1,
        .base = (uint64_t)&gdt[0],
    };
    lgdt(&gdtr);

    // Check loaded GDTR
    struct dtr loaded_gdtr = {
        .limit = 0xffff,
        .base = 0xffffffffffffffffull,
    };
    sgdt(&loaded_gdtr);
    if (gdtr.limit != loaded_gdtr.limit || gdtr.base != loaded_gdtr.base) {
        printf("Failed to load global descriptor table!\n");
        __asm__ volatile("hlt");
    }

    printf("Loaded global descriptor table.\n");
}
