###############################################################################
#   Multiboot2 Header                                                         #
#   Spec: https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html  #
###############################################################################
.set SCREEN_WIDTH, (800)
.set SCREEN_HEIGHT, (600)
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
    .long SCREEN_WIDTH
    .long SCREEN_HEIGHT
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
# Set aside space for page tables (4KB each) and stack (16KiB)
.section .bss
.align 0x1000
pml4:
    .skip 0x1000
pdpt:
    .skip 0x1000
# Each PD points to a PT. Each PT holds 512 entries * 2M pages = 1G of memory mapped.
pd0:
    .skip 0x1000
pd1:
    .skip 0x1000
pd2:
    .skip 0x1000
pd3:
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
    mov %ebx, (mb2_info)
    # %eax contains MB2 magic number
    mov %eax, (mb2_magic)

    # Now we setup paging.
    mov $pml4, %edi
    # 2^14 B needed / 2^4 = 2^10 = 1024 iterations for stosl
    mov $1024, %ecx
    # Initialise page table.
    xor %eax, %eax
    cld
    rep stosl

    # Now we want to point PML4->PDPT->PD
    # Build PML4 (Page Map Level 4)
    mov $pdpt, %eax
    # flags = present | writable
    or $0x3, %eax
    mov %eax, (%edi)

    # Build PDPT (Page Descriptor Pointer Table)
    mov $pdpt, %eax
    or $0x3, %eax
    mov %eax, (pml4)

    ### Build PD0 (0 - 1G) ###
    mov $pd0, %eax
    or $0x3, %eax
    mov %eax, (pdpt)        # Point PDPT0 entry -> PD0
    # Map page table entries (512 * 2M)
    mov $0, %ecx
1:
    mov $0x200000, %eax     # Each huge page is 2M
    mul %ecx                # Multiply by counter to calculate page offset
    or $0b10000011, %eax    # Set flags PRESENT | WRITABLE
    mov %eax, pd0(,%ecx,8)  # Load calc'd page offset into nth PD entry
    inc %ecx
    cmp $512, %ecx
    jb 1b                   # Loop until 512 entries filled for this PD
    ### PD0 sanity check - total mapped should be 1G ###
    mov $0x200000, %eax
    mul %ecx
    # assert eax == 0x40000000 (1G)
    cmp $0x40000000, %eax
    jne boot_err

    ### Build PD1 (1G - 2G) ###
    mov $pd1, %eax
    or $0x3, %eax
    mov %eax, (pdpt + 8)    # Point PDPT1 entry -> PD1 (each PDPTE is 8B)
    # Map page table entries (512 * 2M)
    mov $0, %ecx
1:
    mov $0x200000, %eax     # Each huge page is 2M
    mul %ecx                # Multiply by counter to calculate local page offset
    add $0x40000000, %eax   # This PD starts at 1G
    or $0b10000011, %eax    # Set flags PRESENT | WRITABLE
    mov %eax, pd1(,%ecx,8)  # Load calc'd page offset into nth PD entry
    inc %ecx
    cmp $512, %ecx    # Counter is carried over from last PD
    jb 1b                   # Loop until 512 entries filled for this PD
    ### PD1 sanity check - total mapped should be 2G ###
    mov $0x200000, %eax
    mul %ecx
    add $0x40000000, %eax   # This PD starts at 1G
    # assert eax == 0x80000000 (2G)
    cmp $0x80000000, %eax
    jne boot_err

    ### Build PD2 (2G - 3G) ###
    mov $pd2, %eax
    or $0x3, %eax
    mov %eax, (pdpt + 16)    # Point PDPT2 entry -> PD2
    # Map page table entries (512 * 2M)
    mov $0, %ecx
1:
    mov $0x200000, %eax     # Each huge page is 2M
    mul %ecx                # Multiply by counter to calculate page offset
    add $0x80000000, %eax   # This PD starts at 2G
    or $0b10000011, %eax    # Set flags PRESENT | WRITABLE
    mov %eax, pd2(,%ecx,8)  # Load calc'd page offset into nth PD entry
    inc %ecx
    cmp $512, %ecx
    jb 1b                   # Loop until 512 entries filled for this PD
    ### PD2 sanity check - total mapped should be 3G ###
    mov $0x200000, %eax
    mul %ecx
    add $0x80000000, %eax   # This PD starts at 1G
    # assert eax == 0xc0000000 (3G)
    cmp $0xc0000000, %eax
    jne boot_err

    ### Build PD3 (3G - 4G) ###
    mov $pd3, %eax
    or $0x3, %eax
    mov %eax, (pdpt + 24)    # Point PDPT3 entry -> PD3
    # Map page table entries (512 * 2M)
    mov $0, %ecx
1:
    mov $0x200000, %eax     # Each huge page is 2M
    mul %ecx                # Multiply by counter to calculate page offset
    add $0xc0000000, %eax   # This PD starts at 3G
    or $0b10000011, %eax    # Set flags PRESENT | WRITABLE
    mov %eax, pd3(,%ecx,8)  # Load calc'd page offset into nth PD entry
    inc %ecx
    cmp $512, %ecx
    jb 1b                   # Loop until 512 entries filled for this PD
    ### PD3 sanity check - total mapped should be 4G ###
    mov $0x200000, %eax
    mul %ecx
    add $0xc0000000, %eax   # This PD starts at 3G
    # assert eax == 0x100000000 (4G)
    cmp $0x100000000, %eax
    jne boot_err

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

//     # Framebuffer test
//     mov $0, %ecx
//     mov $0x00ff0000, %edx       # ARGB
// 1:
//     mov $0xfd000000, %edi       # framebuffer addr
//     mov %edx, (%edi, %ecx, 4)   # draw argb[%edx] to framebuffer[%edi]
//     inc %ecx
//     inc %edx
//     cmp $(1920 * 1080), %ecx
//     jne 1b
    // hlt

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
    # RSOD
    mov $0, %ecx
    mov $0x00ff0000, %edx       # ARGB
1:
    mov $0xfd000000, %edi       # framebuffer addr
    mov %edx, (%edi, %ecx, 4)   # draw argb[%edx] to framebuffer[%edi]
    inc %ecx
    cmp $(SCREEN_WIDTH * SCREEN_HEIGHT), %ecx
    jne 1b
    hlt
