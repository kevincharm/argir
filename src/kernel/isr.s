.align 4

.macro ISR_WRAPPER irqno
    .global isr\irqno
    isr\irqno:
        pushl $0        # err_code
        pushl $\irqno   # int_no
        pushal
        cld
        call irq_handler
        popal
        add $8, %esp    # SP+(int_no+err_code)
        iret
.endm

.macro ISR_WRAPPER_WITH_ERR irqno
    .global isr\irqno
    isr\irqno:
        pushl $\irqno   # int_no
        pushal
        cld
        call irq_handler
        popal
        add $8, %esp    # SP+(int_no+err_code)
        iret
.endm

.global isr_systick
isr_systick:
    iret

.global isr_stub
isr_stub:
    call irq_stub_handler
    iret

ISR_WRAPPER 0
ISR_WRAPPER 1
ISR_WRAPPER 2
ISR_WRAPPER 3
ISR_WRAPPER 4
ISR_WRAPPER 5
ISR_WRAPPER 6
ISR_WRAPPER 7
ISR_WRAPPER_WITH_ERR 8
ISR_WRAPPER 9
ISR_WRAPPER_WITH_ERR 10
ISR_WRAPPER_WITH_ERR 11
ISR_WRAPPER_WITH_ERR 12
ISR_WRAPPER_WITH_ERR 13
ISR_WRAPPER_WITH_ERR 14
ISR_WRAPPER 15
ISR_WRAPPER 16
ISR_WRAPPER_WITH_ERR 17
ISR_WRAPPER 18
ISR_WRAPPER 19
ISR_WRAPPER 20
ISR_WRAPPER 21
ISR_WRAPPER 22
ISR_WRAPPER 23
ISR_WRAPPER 24
ISR_WRAPPER 25
ISR_WRAPPER 26
ISR_WRAPPER 27
ISR_WRAPPER 28
ISR_WRAPPER 29
ISR_WRAPPER 30
ISR_WRAPPER 31
ISR_WRAPPER 33 # IRQ1
