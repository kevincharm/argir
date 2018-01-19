#include <stdio.h>
#include <kernel/io.h>
#include <kernel/cpu.h>

void pic_remap()
{
    outb(PIC1_PORT_CMD, 0x11);
    outb(PIC2_PORT_CMD, 0x11);

    outb(PIC1_PORT_DATA, 0x20); // PIC1 -> 32
    outb(PIC2_PORT_DATA, 0x28); // PIC2 -> 40

    outb(PIC1_PORT_DATA, 0x04);
    outb(PIC2_PORT_DATA, 0x02);

    outb(PIC1_PORT_DATA, 0x01);
    outb(PIC2_PORT_DATA, 0x01);

    outb(PIC1_PORT_DATA, 0xff);
    outb(PIC2_PORT_DATA, 0xff);

    printf("Remapped PIC.\n");
}

void pic_irq_off(unsigned int irq_no)
{
    uint16_t port = PIC1_PORT_DATA; // IRQ0-IRQ7
    if (irq_no <= 15) {
        if (irq_no >= 8) {
            port = PIC2_PORT_DATA; // IRQ8-IRQ15
            irq_no -= 8;
        }
        outb(port, inb(port) | (1 << irq_no));
    }
}

void pic_irq_on(unsigned int irq_no)
{
    uint16_t port = PIC1_PORT_DATA; // IRQ0-IRQ7
    if (irq_no <= 15) {
        if (irq_no >= 8) {
            port = PIC2_PORT_DATA; // IRQ8-IRQ15
            irq_no -= 8;
        }
        outb(port, inb(port) & ~(1 << irq_no));
    }
}
