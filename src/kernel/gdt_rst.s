.global gdt_rst
gdt_rst:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    jmp $0x8, $code_seg # Far jump to 0x8 (CS)
code_seg:
    ret
