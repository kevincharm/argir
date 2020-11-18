.align 4
.code64
.section .text
.macro PUSHA
    push %rax
    push %rcx
    push %rdx
    push %rbx
    push %rsp
    push %rbp
    push %rsi
    push %rdi
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15
.endm

.macro POPA
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdi
    pop %rsi
    pop %rbp
    pop %rsp
    pop %rbx
    pop %rdx
    pop %rcx
    pop %rax
.endm

.macro ISR_WRAPPER int_no
    .global isr\int_no
    isr\int_no:
        push $0             # Push a dummy err code so we can use the same interrupt frame struct
        push $\int_no       # Interrupt number
        PUSHA

        # SysV ABI: Clear the string direction flag on interrupt
        cld

        # Call the generic ISR
        mov %rsp, %rdi      # 1st arg <- sp (pointer to interrupt frame)
        call isr_handler

        POPA
        add $16, %rsp       # We also pushed: dummy err code, interrupt number
        iretq
.endm

.macro ISR_WRAPPER_WITH_ERR int_no
    .global isr\int_no
    isr\int_no\():
        # !!! CPU pushes an error code here !!!
        push $\int_no       # int_no
        PUSHA

        # SysV ABI: Clear the string direction flag on interrupt
        cld

        mov %rsp, %rdi      # 1st arg <- sp (pointer to interrupt frame)
        call isr_handler

        POPA
        add $16, %rsp       # The processor pushed an err code, we pushed an interrupt number
        iretq
.endm

.global isr_systick
isr_systick:
    # Systick is IRQ0, so send an EOI (¡muy importante!)
    mov $0x20, %rdi
    call pic_eoi
    iretq

.global isr_stub
isr_stub:
    push $0xff              # dummy err code
    push $0xff              # dummy int_no
    PUSHA

    cld

    mov %rsp, %rdi
    call isr_stub_handler

    POPA
    add $16, %rsp
    iretq

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
ISR_WRAPPER 33              # IRQ1
