.global isr_wrapper
.align 4

isr_wrapper:
    ljmpw $0xffff, $0x0000
    pushal
    cld
    call irq0_handler
    popal
    iret
