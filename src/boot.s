###############################################################################
#   Multiboot2 Header                                                         #
#   Spec: https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html  #
###############################################################################
.set FLAGS, (0)
.set MAGIC, (0xe85250d6)
.set CHECKSUM, -(MAGIC + FLAGS + (mb2_header_end - mb2_header_start))
.section .multiboot
.align 8
mb2_header_start:
    # Fields
    .long MAGIC
    .long FLAGS
    .long mb2_header_end - mb2_header_start
    .long CHECKSUM
.align 8
mb2_tag_fb_start:
    # Framebuffer tag (MB2 Spec, Section 3.1.10)
    .short 5
    .short 0
    .long mb2_tag_fb_end - mb2_tag_fb_start
    .long 1920                      # width
    .long 1080                      # height
    .long 32                        # depth (bits per pixel)
mb2_tag_fb_end:
.align 8
mb2_tag_null_start:
    # Empty tag, 8-byte aligned
    .short 0
    .short 0
    .long mb2_header_end - mb2_tag_null_start
mb2_header_end:

###############################################################################
#   Protected mode -> long mode                                               #
###############################################################################
# Set aside space for page tables (16KiB) and stack (16KiB)
.section .bss
.align 0x1000
pml4:
    .skip 0x1000
pdpt:
    .skip 0x1000
pd:
    .skip 0x1000
.align 4
stack_bottom:
    .skip 16384
stack_top:

.section .rodata
.align 4
empty_idt:
    .short 0x0  # length
    .int 0x0    # base
### BEGIN 64-bit GDT ###
# Descriptor entries
gdt:
gdt_null_desc:
    .quad 0x0                       # null descriptor
gdt_code_desc:
    .quad 0x00209a0000000000        # kernel code descriptor r-x
gdt_data_desc:
    .quad 0x0000920000000000        # kernel data descriptor rw-
# GDT struct
p_gdt:
    .short (p_gdt - gdt - 1)        # limit
    .quad (gdt)                     # base
# Segment pointers
.set gdt_code_seg, gdt_code_desc - gdt
.set gdt_data_seg, gdt_data_desc - gdt
### END 64-bit GDT ###

.extern kernel_main
.section .text
.code32
.global _start
.type _start, @function
_start:
    mov $stack_top, %esp

    # Reset EFLAGS
    pushl $0
    popf

    # %ebx contains the physical address of MB2 info structure.
    push %ebx
    # %eax contains MB2 magic number
    push %eax

    # TODO: First we should:
    # 1. Check that Multiboot2 dropped us here.
    # 2. Check that long mode is actually available.
    # But it's 2020 so we just yolo it.

    # Now we setup paging.
    mov $pml4, %edi
    # 2^14 B needed / 2^4 = 2^10 = 1024 iterations for stosl
    mov $1024, %ecx
    # Initialise page table.
    xor %eax, %eax
    cld
    rep stosl

    # Now we want to point PML4->PDPT->PD->PT
    # Build PML4 (Page Map Level 4)
    # First PML4 entry lives at es:di. PML4 table is 16KiB long.
    # 1. Load the first PDPT entry into eax,
    # 2. Set page present and writable flag.
    # 3. Load eax into es:di.
    # 4. Repeat for PDPT and PD.
    mov $pdpt, %eax
    # flags = present | writable
    or $0x3, %eax
    mov %eax, (%edi)

    # Build PDPT (Page Descriptor Pointer Table)
    mov $pdpt, %eax
    or $0x3, %eax
    mov %eax, (pml4)

    # Build PD (Page Directory, 2MiB)
    mov $pd, %eax
    or $0x3, %eax
    mov %eax, (pdpt)

    # Map page table entries (512 * 2M)
    mov $0, %ecx
pt_loop:
    mov $0x200000, %eax
    mul %ecx
    or $0b10000011, %eax
    mov %eax, pd(,%ecx,)
    add $8, %ecx
    cmp $(512 * 8), %ecx
    jb pt_loop

    # Load empty IDT
    lidt (empty_idt)

    # Enable PAE and PGE in CR4.
    mov %cr4, %eax
    or $((1 << 5) | (1 << 7)), %eax
    mov %eax, %cr4

    # Set CR3 to our PML4 table.
    mov $pml4, %eax
    mov %eax, %cr3

    # We need to write to the MSR to set the LME (Long Mode Enable) bit.
    # `rdmsr` reads from the MSR specified in %ecx into %edx:%eax.
    # `wrmsr` writes %edx:%eax into the MSR specified in %ecx.
    # IA32_EFER number is 0xc0000080.
    mov $0xc0000080, %ecx
    rdmsr
    or $(1 << 8), %eax
    wrmsr

    # Finally, set PE and PG bit in CR0 to activate long mode.
    mov %cr0, %eax
    or $(1 << 31), %eax
    mov %eax, %cr0

    # Load the GDT.
    lgdt (p_gdt)

    # Set all the segments to the data segment.
    mov $gdt_data_seg, %ax
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    mov %ax, %ds
    mov %ax, %es

    # Here we go bois.
    ljmp $gdt_code_seg, $kernel_main

    cli
1:  hlt
    jmp 1b

boot_err:
    movl $0x1f421f44, (0xb8000)
    movl $0x1f3d1f47, (0xb8004)
    and $0x000000ff, %eax
    or $0x00000f00, %eax
    movl %eax, (0xb8008)
    hlt
