#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <kernel/cpu.h>
#include <kernel/interrupts.h>
#include <kernel/pic.h>

#define IDT_DEFAULT_ISR_HANDLER(n)                                             \
    extern void isr##n(void);                                                  \
    set_interrupt_desc(n, (uint32_t)isr##n);

extern void isr_stub(void);
extern void keyboard_irq_handler(void);

void isr_handler()
{
    // __asm__ volatile("mov $0xfeedface, %rax\n\t"
    //                  "movq $0x123abc, 0x0\n\t");
    // printf("interrupt received\n");
    // if (frame.int_no == 33) {
    //     keyboard_irq_handler();
    // }

    // pic_eoi(frame.int_no);
}

/**
 * Stub handler.
 */
void isr_stub_handler()
{
    // __asm__ volatile("mov $0xfeedface, %rax\n\t"
    //                  "movq $0x123abc, 0x0\n\t");
    // (void)frame;
    // printf("IRQ stub handler called!\n");
}

void interrupts_init()
{
    // Initialise the IDT
    for (size_t i = 0; i < IDT_ENTRIES_COUNT; i++)
        memset(&idt[i], 0, sizeof(struct gate_desc));

    // Intel-reserved interrupts
    printf("Installing ISRs...\n");
    IDT_DEFAULT_ISR_HANDLER(0); // #DE (Div-by-zero)
    IDT_DEFAULT_ISR_HANDLER(1); // #DB (Debug trap)
    IDT_DEFAULT_ISR_HANDLER(2); // NMI
    IDT_DEFAULT_ISR_HANDLER(3); // #BP (Breakpoint)
    IDT_DEFAULT_ISR_HANDLER(4); // #OF (Overflow)
    IDT_DEFAULT_ISR_HANDLER(5); // #BR (Out-of-bounds)
    IDT_DEFAULT_ISR_HANDLER(6); // #UD (Undefined instruction)
    IDT_DEFAULT_ISR_HANDLER(7); // #NM (Device not available)
    IDT_DEFAULT_ISR_HANDLER(8); // #DF (Double Fault)
    IDT_DEFAULT_ISR_HANDLER(9);
    IDT_DEFAULT_ISR_HANDLER(10);
    IDT_DEFAULT_ISR_HANDLER(11);
    IDT_DEFAULT_ISR_HANDLER(12);
    IDT_DEFAULT_ISR_HANDLER(13);
    IDT_DEFAULT_ISR_HANDLER(14);
    IDT_DEFAULT_ISR_HANDLER(15);
    IDT_DEFAULT_ISR_HANDLER(16);
    IDT_DEFAULT_ISR_HANDLER(17);
    IDT_DEFAULT_ISR_HANDLER(18);
    IDT_DEFAULT_ISR_HANDLER(19);
    IDT_DEFAULT_ISR_HANDLER(20);
    IDT_DEFAULT_ISR_HANDLER(21);
    IDT_DEFAULT_ISR_HANDLER(22);
    IDT_DEFAULT_ISR_HANDLER(23);
    IDT_DEFAULT_ISR_HANDLER(24);
    IDT_DEFAULT_ISR_HANDLER(25);
    IDT_DEFAULT_ISR_HANDLER(26);
    IDT_DEFAULT_ISR_HANDLER(27);
    IDT_DEFAULT_ISR_HANDLER(28);
    IDT_DEFAULT_ISR_HANDLER(29);
    IDT_DEFAULT_ISR_HANDLER(30);
    IDT_DEFAULT_ISR_HANDLER(31);

    // Redirect the rest of the IDT entries to the isr stub handler as a sane default
    for (size_t i = 0; i < 256; i++) {
        set_interrupt_desc(i, isr_stub);
    }

    // Remap PIC
    pic_remap();

    // Enable IRQ1 (PS/2)
    // IDT_DEFAULT_ISR_HANDLER(33);
    pic_enable_all_irqs();
    // pic_enable_only_keyboard();
    // pic_irq_on(1);

    idt_init();
}
