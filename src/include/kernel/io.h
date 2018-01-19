#ifndef __ARGIR__IO_H
#define __ARGIR__IO_H

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t byte)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a" (byte), "Nd" (port)
    );
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile (
        "inb %1, %0"
        : "=r" (ret)
        : "Nd" (port)
    );
    return ret;
}

static inline void io_wait()
{
    outb(0x80, 0x0);
}

#endif /* __ARGIR__IO_H */
