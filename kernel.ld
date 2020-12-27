OUTPUT_FORMAT("elf64-x86-64")

ENTRY(_start)

/** Leave 2M for bootloader */
KERNEL_LMA = 0x200000;
/** Kernel will live at -2G + 2M */
KERNEL_VMA = 0xffffffff80000000;

SECTIONS
{
    . = KERNEL_LMA + KERNEL_VMA;

    _kernel_start = .;

    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VMA)
    {
        KEEP(*(.multiboot))
        *(.text)
    }

    /* Read-only data. */
    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VMA)
    {
        *(.rodata)
    }

    /* Read-write data (initialised) */
    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VMA)
    {
        *(.data)
    }

    /* Read-write data (uninitialised) and stack */
    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VMA)
    {
        *(COMMON)
        *(.bss)
    }

    _kernel_end = .;
}
