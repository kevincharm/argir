#ifndef __ARGIR__CPU_H
#define __ARGIR__CPU_H

#include <stddef.h>
#include <stdint.h>

/**
 *  Global Descriptor Table
 */
#define GDT_ENTRIES_COUNT (3)

/**
 * Generic segment descriptor (long mode)
 * AMD64 APM p.80
 */
struct gen_seg_desc {
    uint16_t limit_lo : 16;
    uint16_t base_lo : 16;
    uint8_t base_mid : 8;
    uint8_t access : 8;
    uint8_t limit_hi_gran : 8;
    uint8_t base_hi : 8;
} __attribute__((packed));

/**
 * Code segment descriptor (long mode)
 * AMD64 APM p.82
 */
struct code_seg_desc {
    uint16_t limit_lo;
    uint16_t base_lo;
    uint8_t base_mid;
    uint8_t a : 1;
    uint8_t r : 1;
    uint8_t c : 1;
    uint8_t one : 1;
    uint8_t another_one : 1; /** D-D-D-DJ Khaled */
    uint8_t dpl : 2;
    uint8_t p : 1;
    uint8_t limit_hi : 4;
    uint8_t avl : 1;
    uint8_t l : 1;
    uint8_t d : 1;
    uint8_t g : 1;
    uint8_t base_hi;
} __attribute__((packed));

/**
 * Data segment descriptor
 * AMD64 APM p.83
 */
struct data_seg_desc {
    uint16_t limit_lo : 16;
    uint16_t base_lo : 16;
    uint8_t base_mid : 8;
    uint8_t a : 1;
    uint8_t w : 1;
    uint8_t e : 1;
    uint8_t zero : 1;
    uint8_t one : 1;
    uint8_t dpl : 2;
    uint8_t p : 1;
    uint8_t limit_hi : 4;
    uint8_t avl : 1;
    uint8_t l : 1;
    uint8_t db : 1;
    uint8_t g : 1;
    uint8_t base_hi : 8;
} __attribute__((packed));

struct dtr {
    volatile uint16_t limit;
    volatile uint64_t base;
} __attribute__((packed));

struct gen_seg_desc gdt[GDT_ENTRIES_COUNT] __attribute__((align(4096)));

void gdt_load();
void gdt_init();

/**
 *  Interrupt Descriptor Table
 */
#define IDT_ENTRIES_COUNT (256)
#define IDT_CODE_SEGMENT (0x8)

/**
 * System-segment descriptor types (long mode)
 * AMD64 APM p.90
 */
#define SSDT_INTERRUPT_GATE (0xe)
#define SSDT_TRAP_GATE (0xf)

/**
 * Interrupt-gate and trap-gate descriptor (long mode)
 * AMD64 APM p.93
 */
struct gate_desc {
    uint64_t offset_lo : 16;
    uint64_t selector : 16;
    uint64_t ist : 3;
    uint64_t reserved_1 : 5;
    uint64_t type : 4;
    uint64_t zero : 1;
    uint64_t dpl : 2;
    uint64_t p : 1;
    uint64_t offset_hi : 48;
    uint64_t reserved_2 : 32;
} __attribute__((__packed__));

struct gate_desc idt[IDT_ENTRIES_COUNT] __attribute__((align(4096)));

/**
 * Register an interrupt handler with address `base`.
 */
void set_interrupt_desc(size_t index, uint64_t base);
void idt_init();

#endif /* __ARGIR__CPU_H */
