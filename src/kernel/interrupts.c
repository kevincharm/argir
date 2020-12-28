#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <kernel/cpu.h>
#include <kernel/interrupts.h>
#include <kernel/pic.h>

#define IDT_DEFAULT_ISR_HANDLER(n)                                             \
    extern void isr##n(void);                                                  \
    set_interrupt_desc(n, isr##n);

extern void isr_systick(void);
extern void isr_stub(void);
extern void keyboard_irq_handler(void);

/**
 * Generic ISR.
 * TODO: Use separate ISRs instead of single ISR + branching.
 */
void isr_handler(struct interrupt_frame *frame)
{
    switch (frame->int_no) {
    case 0: // #DE
        printf("Fault occurred: Divide-by-zero\n");
        break;
    case 6: // #UD (Invalid Opcode)
        printf("Fault occurred: Invalid opcode\n");
        break;
    case 8: // #DF (Double Fault)
        printf("Fault occurred: Double fault (0x%x)", frame->err_code);
        __asm__ volatile("1: jmp 1b");
        break;
    case 13: // #GP (General Protection Fault)
        printf("Fault occurred: General protection fault (0x%x)\n",
               frame->err_code);
        break;
    case 14: // #PF (Page Fault)
        printf("Fault occurred: Page fault (0x%x)\n", frame->err_code);
        break;
    case 33: // 0x21
        keyboard_irq_handler();
        break;
    }

    // Send an EOI iff (!!) this ISR was triggered by an IRQ
    // IRQ 0-7 from PIC1 = [0x20, 0x28)
    // IRQ 8-15 from PIC2 = [0x28, 0x30)
    if (frame->int_no >= 0x20 && frame->int_no < 0x30) {
        pic_eoi(frame->int_no);
    }
}

/**
 * Stub handler.
 */
void isr_stub_handler(struct interrupt_frame *frame)
{
    printf("IRQ stub handler called! (int_no: 0x%x)\n", frame->int_no);
}

void interrupts_init()
{
    // Initialise the IDT
    for (size_t i = 0; i < IDT_ENTRIES_COUNT; i++)
        memset(&idt[i], 0, sizeof(struct gate_desc));

    // Remap PIC, leaves all IRQs masked.
    pic_remap();

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
    set_interrupt_desc(32, isr_systick); // IRQ0: Timer
    IDT_DEFAULT_ISR_HANDLER(33); // IRQ1: PS/2 Keyboard

    // Redirect the rest of the IDT entries to the isr stub handler as a sane default
    for (size_t i = 34; i < 256; i++) {
        set_interrupt_desc(i, isr_stub);
    }

    // Unmask the hardware IRQs we want to know about
    pic_enable_all_irqs();

    idt_init();
}
