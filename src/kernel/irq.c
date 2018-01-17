#include <stddef.h>
#include <stdio.h>
#include <kernel/irq.h>
#include <kernel/io.h>

struct idt_entry idt[256];
void idt_entry_set(struct idt_entry *entry, uint32_t base, uint8_t selector, uint8_t flags)
{
    entry->base_lo = (base >>  0u) & 0xffff;
    entry->base_hi = (base >> 16u) & 0xffff;
    entry->selector = selector;
    entry->flags = flags;
    entry->unused = 0;
}

#define PIC1_PORT_CMD   (0x20)
#define PIC1_PORT_DATA  (PIC1_PORT_CMD+1)
#define PIC2_PORT_CMD   (0xA0)
#define PIC2_PORT_DATA  (PIC2_PORT_CMD+1)

static void pic_remap()
{
    outb(PIC1_PORT_CMD, 0x11);
    io_wait();
    outb(PIC2_PORT_CMD, 0x11);
    io_wait();

    outb(PIC1_PORT_DATA, 0x20); // PIC1 -> 32
    io_wait();
    outb(PIC2_PORT_DATA, 0x28); // PIC2 -> 40
    io_wait();

    outb(PIC1_PORT_DATA, 0x00);
    io_wait();
    outb(PIC2_PORT_DATA, 0x00);
    io_wait();

    outb(PIC1_PORT_DATA, 0x01);
    io_wait();
    outb(PIC2_PORT_DATA, 0x01);
    io_wait();

    outb(PIC1_PORT_DATA, 0xff);
    io_wait();
    outb(PIC2_PORT_DATA, 0xff);
    io_wait();
}

extern void isr_wrapper(void);
extern bool irq_test;
extern void keyboard_irq_handler(void);
void irq0_handler(struct interrupt_frame *frame)
{
    printf("-> irq0\n");
    (void)frame;
    // keyboard_irq_handler();
    irq_test = true;

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
    // TODO: init idt table with memset
    uint8_t sel = IDT_SEL_KERNEL;
    uint8_t flags = IDT_FLAGS_BASE | ((0x0 & 0x3) << 4u) | (1u << 7u);
    idt_entry_set(idt+33, (uint32_t)isr_wrapper, sel, flags); // IRQ1 + PIC1 offset (=32)

    pic_remap();
    outb(PIC1_PORT_DATA, 0xfb); // IRQ1
    printf("Remapped PIC.\n");
    lidt(&idt, (sizeof (struct idt_entry) * 256) - 1);
    printf("Loaded interrupt descriptor table.\n");
}
