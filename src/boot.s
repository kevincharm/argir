# Multiboot header
.set ALIGN,     (1 << 0)
.set MEMINFO,   (1 << 1)
.set FLAGS,     (ALIGN | MEMINFO)
.set MAGIC,     (0x1BADB002)
.set CHECKSUM,  -(MAGIC + FLAGS)
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Stack setup, 16KiB uninitialised.
.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:

# Linker script
.section .text
.global _start
.type _start, @function
_start:
    # 32-bit protected mode
    mov $stack_top, %esp

    # TODO: enable cpu extensions before entering kernel

    # Stack must be 16 byte aligned at call instruction.
    call kernel_main

    # Sleep - similar to SEV+WFE+WFE on ARM.
    cli
1:  hlt
    jmp 1b

.size _start, . - _start
