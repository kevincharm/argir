.global isr_wrapper
.align 4

isr_wrapper:
    pushal
    cld
    call irq_handler
    popal
    iret
