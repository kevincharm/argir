#ifndef __ARGIR__INTERRUPTS_H
#define __ARGIR__INTERRUPTS_H

#include <stdbool.h>
#include <stdint.h>

struct interrupt_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rdi, rsi, rbp, rsp, rbx, rdx, rcx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, flags, sp, ss;
};

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
