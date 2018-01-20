#include <stddef.h>
#include <stdio.h>
#include <kernel/irq.h>
#include <kernel/io.h>
#include <kernel/cpu.h>

#define DEFAULT_ISR_HANDLER(n) \
    extern void isr##n(void); \
    idt_entry_set(0, (uint32_t)isr##n, 0x08, 0x8e);

extern void keyboard_irq_handler(void);
void irq_handler(struct irq_frame frame)
{
    printf("%u\n", frame.int_no);

    if (frame.int_no == 33) {
        keyboard_irq_handler();
    }

    if (frame.int_no >= 0x28) {
        // EOI to PIC2 (slave)
        outb(PIC2_PORT_CMD, 0x20);
    }

    // EOI to PIC1 (master)
    outb(PIC1_PORT_CMD, 0x20);
}

void irq_stub_handler(struct irq_frame frame)
{
    printf("IRQ stub handler called!\n");
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

extern void isr_stub(void);
void irq_init()
{
    DEFAULT_ISR_HANDLER(0);
    DEFAULT_ISR_HANDLER(1);
    DEFAULT_ISR_HANDLER(2);
    DEFAULT_ISR_HANDLER(3);
    DEFAULT_ISR_HANDLER(4);
    DEFAULT_ISR_HANDLER(5);
    DEFAULT_ISR_HANDLER(6);
    DEFAULT_ISR_HANDLER(7);
    DEFAULT_ISR_HANDLER(8);
    DEFAULT_ISR_HANDLER(9);
    DEFAULT_ISR_HANDLER(10);
    DEFAULT_ISR_HANDLER(11);
    DEFAULT_ISR_HANDLER(12);
    DEFAULT_ISR_HANDLER(13);
    DEFAULT_ISR_HANDLER(14);
    DEFAULT_ISR_HANDLER(15);
    DEFAULT_ISR_HANDLER(16);
    DEFAULT_ISR_HANDLER(17);
    DEFAULT_ISR_HANDLER(18);
    DEFAULT_ISR_HANDLER(19);
    DEFAULT_ISR_HANDLER(20);
    DEFAULT_ISR_HANDLER(21);
    DEFAULT_ISR_HANDLER(22);
    DEFAULT_ISR_HANDLER(23);
    DEFAULT_ISR_HANDLER(24);
    DEFAULT_ISR_HANDLER(25);
    DEFAULT_ISR_HANDLER(26);
    DEFAULT_ISR_HANDLER(27);
    DEFAULT_ISR_HANDLER(28);
    DEFAULT_ISR_HANDLER(29);
    DEFAULT_ISR_HANDLER(30);
    DEFAULT_ISR_HANDLER(31);

    uint8_t sel = IDT_SEL_KERNEL;
    uint8_t flags = IDT_FLAGS_BASE | ((0x0 & 0x3) << 4u) | (1u << 7u);
    for (size_t i=32; i<256; i++) { // IRQ1+
        idt_entry_set(i, (uint32_t)isr_stub, sel, flags);
    }
    // extern void isr_systick(void);
    // idt_entry_set(32, (uint32_t)isr_systick, sel, flags); // IRQ0
    extern void isr33(void);
    idt_entry_set(33, (uint32_t)isr33, sel, flags);

    printf("Interrupts enabled.\n\n");
}
