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

static inline void outw(uint16_t port, uint16_t word)
{
    __asm__ volatile (
        "outw %0, %1"
        :
        : "a" (word), "Nd" (port)
    );
}

static inline void outl(uint16_t port, uint32_t dword)
{
    __asm__ volatile (
        "outl %0, %1"
        :
        : "a" (dword), "Nd" (port)
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

static inline uint16_t inw(uint16_t port)
{
    uint16_t ret;
    __asm__ volatile (
        "inw %1, %0"
        : "=r" (ret)
        : "Nd" (port)
    );
    return ret;
}

static inline uint32_t inl(uint16_t port)
{
    uint32_t ret;
    __asm__ volatile (
        "inl %1, %0"
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
