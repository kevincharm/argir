#include <stddef.h>
#include <stdio.h>
#include <kernel/cpu.h>
#include <kernel/interrupts.h>
#include <kernel/pic.h>

// #define IDT_DEFAULT_ISR_HANDLER(n)                                             \
//     extern void isr##n(void);                                                  \
//     idt_entry_set(n, (uint32_t)isr##n, IDT_ENTRY_DEFAULT_SEL,                  \
//                   IDT_ENTRY_DEFAULT_FLAG);

// extern void isr_stub(void);
extern void keyboard_irq_handler(void);

void isr_handler(struct interrupt_frame frame)
{
    if (frame.int_no == 33) {
        keyboard_irq_handler();
    }

    pic_eoi(frame.int_no);
}

void isr_stub_handler(struct interrupt_frame frame)
{
    (void)frame;
    printf("IRQ stub handler called!\n");
}

void interrupts_init()
{
    // Intel-reserved interrupts
    pic_remap();
    // IDT_DEFAULT_ISR_HANDLER(0);
    // IDT_DEFAULT_ISR_HANDLER(1);
    // IDT_DEFAULT_ISR_HANDLER(2);
    // IDT_DEFAULT_ISR_HANDLER(3);
    // IDT_DEFAULT_ISR_HANDLER(4);
    // IDT_DEFAULT_ISR_HANDLER(5);
    // IDT_DEFAULT_ISR_HANDLER(6);
    // IDT_DEFAULT_ISR_HANDLER(7);
    // IDT_DEFAULT_ISR_HANDLER(8);
    // IDT_DEFAULT_ISR_HANDLER(9);
    // IDT_DEFAULT_ISR_HANDLER(10);
    // IDT_DEFAULT_ISR_HANDLER(11);
    // IDT_DEFAULT_ISR_HANDLER(12);
    // IDT_DEFAULT_ISR_HANDLER(13);
    // IDT_DEFAULT_ISR_HANDLER(14);
    // IDT_DEFAULT_ISR_HANDLER(15);
    // IDT_DEFAULT_ISR_HANDLER(16);
    // IDT_DEFAULT_ISR_HANDLER(17);
    // IDT_DEFAULT_ISR_HANDLER(18);
    // IDT_DEFAULT_ISR_HANDLER(19);
    // IDT_DEFAULT_ISR_HANDLER(20);
    // IDT_DEFAULT_ISR_HANDLER(21);
    // IDT_DEFAULT_ISR_HANDLER(22);
    // IDT_DEFAULT_ISR_HANDLER(23);
    // IDT_DEFAULT_ISR_HANDLER(24);
    // IDT_DEFAULT_ISR_HANDLER(25);
    // IDT_DEFAULT_ISR_HANDLER(26);
    // IDT_DEFAULT_ISR_HANDLER(27);
    // IDT_DEFAULT_ISR_HANDLER(28);
    // IDT_DEFAULT_ISR_HANDLER(29);
    // IDT_DEFAULT_ISR_HANDLER(30);
    // IDT_DEFAULT_ISR_HANDLER(31);

    // Redirect the rest of the IDT entries to the isr stub handler as a sane default
    // for (size_t i = 32; i < 256; i++) {
    //     idt_entry_set(i, (uint32_t)isr_stub, IDT_ENTRY_DEFAULT_SEL,
    //                   IDT_ENTRY_DEFAULT_FLAG);
    // }

    // IRQ1 (PS/2)
    // IDT_DEFAULT_ISR_HANDLER(33);
    pic_irq_on(1);

    idt_load();
}
