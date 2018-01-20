#include <stddef.h>
#include <stdio.h>
#include <kernel/irq.h>
#include <kernel/io.h>
#include <kernel/cpu.h>

extern void isr_wrapper(void);
extern void keyboard_irq_handler(void);
void irq_handler(struct irq_frame *frame)
{
    printf("a");
    if (frame->int_no == 33) {
        keyboard_irq_handler();
    }

    if (frame->int_no >= 0x28) {
        // EOI to PIC2 (slave)
        outb(PIC2_PORT_CMD, 0x20);
    }

    // EOI to PIC1 (master)
    outb(PIC1_PORT_CMD, 0x20);
}

void set_irq_mask(uint8_t irq_num)
{
    uint16_t port = PIC1_PORT_DATA;
    if (irq_num >= 8) {
        port = PIC2_PORT_DATA;
        irq_num -= 8;
    }

    uint8_t out = inb(port) & ~(1u << irq_num);
    outb(port, out);
}

void irq_init()
{
    uint8_t sel = IDT_SEL_KERNEL;
    uint8_t flags = IDT_FLAGS_BASE | ((0x0 & 0x3) << 4u) | (1u << 7u);
    for (size_t i=0; i<256; i++) {
        idt_entry_set(i, (uint32_t)isr_wrapper, sel, flags);
    }
    printf("Interrupts enabled.\n\n");
}
