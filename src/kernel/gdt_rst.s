.section .text
.code64
.global gdt_rst
gdt_rst:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    sub $16, %rsp
    movq $8, 8(%rsp)
    movabsq $code_seg, %rax
    mov %rax, (%rsp)
    lretq
code_seg:
    ret
