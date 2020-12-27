#ifndef __ARGIR__INTERRUPTS_H
#define __ARGIR__INTERRUPTS_H

#include <stdbool.h>
#include <stdint.h>

struct interrupt_frame {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t rbx;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rax;
    uint64_t int_no;
    uint64_t err_code;
} __attribute__((packed)); /** Redundant? This should have no padding anyway */

static inline void interrupts_enable()
{
    __asm__("sti");
}

static inline void interrupts_disable()
{
    __asm__("cli");
}

static inline bool interrupts_enabled()
{
    uint32_t flags;
    __asm__ volatile("pushf\n\t"
                     "pop %0"
                     : "=g"(flags));
    return (flags >> 9u) & 0x1;
}

void interrupts_init();

#endif /* __ARGIR__INTERRUPTS_H */
