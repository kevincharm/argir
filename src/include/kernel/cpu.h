#ifndef __ARGIR__CPU_H
#define __ARGIR__CPU_H

#include <stddef.h>
#include <stdint.h>

/**
 *  Global Descriptor Table
 */
#define GDT_ENTRIES_COUNT       (3)

struct gdt_entry {
    uint16_t limit_lo;
    uint16_t base_lo;
    uint8_t base_mid;
    uint8_t access;
    uint8_t gran;
    uint8_t base_hi;
}__attribute__((packed));

struct gdtr {
    uint16_t limit;
    uint32_t base;
}__attribute__((packed));

struct gdt_entry gdt[GDT_ENTRIES_COUNT];

void gdt_entry_set(size_t index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void gdt_load();
void gdt_init();

/**
 *  Interrupt Descriptor Table
 */
#define IDT_ENTRIES_COUNT       (256)
#define IDT_SEL_KERNEL          (0x8)
#define IDT_FLAGS_BASE          (0xe)
#define IDT_ENTRY_DEFAULT_SEL   (IDT_SEL_KERNEL)
#define IDT_ENTRY_DEFAULT_FLAG  (IDT_FLAGS_BASE | ((0x0 & 0x3) << 4u) | (1u << 7u))

struct idt_entry {
    uint16_t base_lo;
    uint16_t selector;
    uint8_t unused;
    uint8_t flags;
    uint16_t base_hi;
}__attribute__((packed));

struct idtr {
    uint16_t limit;
    uint32_t base;
}__attribute__((packed));

struct idt_entry idt[IDT_ENTRIES_COUNT];

void idt_entry_set(size_t index, uint32_t base, uint8_t selector, uint8_t flags);
void idt_load();

#endif /* __ARGIR__CPU_H */
