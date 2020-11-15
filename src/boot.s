.set KERNEL_LMA, (0x200000)
.set KERNEL_VMA, (0xffffffff80000000 + KERNEL_LMA)

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
# Identity mapped level 4 paging, mirrored at -2G
pml4:
    .skip 0x1000
pdpt:
    .skip 0x1000
# Since we're using hugepages, each PD entry (8B) points to a 2M page.
# -> Each PD holds 0x1000 / 8 = 512 entries.
# -> Each PD maps 512 * 2M = 2^30 = 1GiB of memory.
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
    .int 0x0 - KERNEL_VMA    # base
### BEGIN 64-bit GDT ###
# Descriptor entries
.align 4
gdt:
gdt_null_desc:
    .quad 0x0                       # null descriptor
gdt_code_desc:
    .quad 0x00209a0000000000        # kernel code descriptor r-x
gdt_data_desc:
    .quad 0x0000920000000000        # kernel data descriptor rw-
gdt_end:
# GDT struct
p_gdt32:
    .short (gdt_end - gdt - 1)      # limit
    .long gdt - KERNEL_VMA          # base
p_gdt64:
    .short (gdt_end - gdt - 1)      # limit
    .quad gdt                       # base
# Segment pointers
.set gdt_code_seg, gdt_code_desc - gdt
.set gdt_data_seg, gdt_data_desc - gdt
### END 64-bit GDT ###

.extern kernel_main
.extern mb2_info
.extern mb2_magic
.section .text
.code32
.global _start
.type _start, @function
_start:
    mov $(stack_top - KERNEL_VMA), %esp

    # Reset EFLAGS
    pushl $0
    popf

    # %ebx contains the physical address of MB2 info structure.
    mov %ebx, (mb2_info - KERNEL_VMA)
    # %eax contains MB2 magic number
    mov %eax, (mb2_magic - KERNEL_VMA)

    # Now we setup paging.
    mov $(pml4 - KERNEL_VMA), %edi
    # 2^14 B needed / 2^4 = 2^10 = 1024 iterations for stosl
    mov $1024, %ecx
    # Initialise page table.
    xor %eax, %eax
    cld
    rep stosl

    # Build PDPT (Page Descriptor Pointer Table)
    mov $(pdpt - KERNEL_VMA), %eax
    or $0x3, %eax
    mov %eax, (pml4 - KERNEL_VMA)
    mov %eax, (pml4 + (511 * 8) - KERNEL_VMA)   # Mirror at -2G (PML4[511])

    ### Build PD0 (0 - 1G) ###
    mov $(pd0 - KERNEL_VMA), %eax
    or $0x3, %eax
    mov %eax, (pdpt - KERNEL_VMA)               # Point PDPT0 entry -> PD0
    mov %eax, (pdpt + (510 * 8) - KERNEL_VMA)   # First 1G of physical memory maps to PDPT[510]
    # Map page table entries (512 * 2M)
    mov $0, %ecx
1:
    mov $0x200000, %eax     # Each huge page is 2M
    mul %ecx                # Multiply by counter to calculate page offset
    or $0b10000011, %eax    # Set flags PRESENT | WRITABLE
    mov %eax, (pd0 - KERNEL_VMA)(,%ecx,8)  # Load calc'd page offset into nth PD entry
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
    mov $(pd1 - KERNEL_VMA), %eax
    or $0x3, %eax
    mov %eax, (pdpt + 8 - KERNEL_VMA)           # Point PDPT1 entry -> PD1 (each PDPTE is 8B)
    mov %eax, (pdpt + (511 * 8) - KERNEL_VMA)   # 1G-2G of physical memory maps to PDPT[511]
    # Map page table entries (512 * 2M)
    mov $0, %ecx
1:
    mov $0x200000, %eax     # Each huge page is 2M
    mul %ecx                # Multiply by counter to calculate local page offset
    add $0x40000000, %eax   # This PD starts at 1G
    or $0b10000011, %eax    # Set flags PRESENT | WRITABLE
    mov %eax, (pd1 - KERNEL_VMA)(,%ecx,8)  # Load calc'd page offset into nth PD entry
    inc %ecx
    cmp $512, %ecx
    jb 1b                   # Loop until 512 entries filled for this PD
    ### PD1 sanity check - total mapped should be 2G ###
    mov $0x200000, %eax
    mul %ecx
    add $0x40000000, %eax   # This PD starts at 1G
    # assert eax == 0x80000000 (2G)
    cmp $0x80000000, %eax
    jne boot_err

    ### Build PD2 (2G - 3G) ###
    mov $(pd2 - KERNEL_VMA), %eax
    or $0x3, %eax
    mov %eax, (pdpt + 16 - KERNEL_VMA)    # Point PDPT2 entry -> PD2
    # Map page table entries (512 * 2M)
    mov $0, %ecx
1:
    mov $0x200000, %eax     # Each huge page is 2M
    mul %ecx                # Multiply by counter to calculate page offset
    add $0x80000000, %eax   # This PD starts at 2G
    or $0b10000011, %eax    # Set flags PRESENT | WRITABLE
    mov %eax, (pd2 - KERNEL_VMA)(,%ecx,8)  # Load calc'd page offset into nth PD entry
    inc %ecx
    cmp $512, %ecx
    jb 1b                   # Loop until 512 entries filled for this PD
    ### PD2 sanity check - total mapped should be 3G ###
    mov $0x200000, %eax
    mul %ecx
    add $0x80000000, %eax   # This PD starts at 2G
    # assert eax == 0xc0000000 (3G)
    cmp $0xc0000000, %eax
    jne boot_err

    ### Build PD3 (3G - 4G) ###
    mov $(pd3 - KERNEL_VMA), %eax
    or $0x3, %eax
    mov %eax, (pdpt + 24 - KERNEL_VMA)    # Point PDPT3 entry -> PD3
    # Map page table entries (512 * 2M)
    mov $0, %ecx
1:
    mov $0x200000, %eax     # Each huge page is 2M
    mul %ecx                # Multiply by counter to calculate page offset
    add $0xc0000000, %eax   # This PD starts at 3G
    or $0b10000011, %eax    # Set flags PRESENT | WRITABLE
    mov %eax, (pd3 - KERNEL_VMA)(,%ecx,8)  # Load calc'd page offset into nth PD entry
    inc %ecx
    cmp $512, %ecx
    jb 1b                   # Loop until 512 entries filled for this PD
    ### PD3 sanity check - total mapped should be 4G ###
    mov $0x200000, %eax
    mul %ecx
    add $0xc0000000, %eax   # This PD starts at 3G
    # assert eax == 0x100000000 (4G)
    cmp $0x100000000, %eax  # Yes, this is an overflow.
    jne boot_err

    # Load empty IDT
    lidt empty_idt - KERNEL_VMA

    # Enable PAE and PGE in CR4.
    mov %cr4, %eax
    or $((1 << 5) | (1 << 7)), %eax
    mov %eax, %cr4

    # Set CR3 to our PML4 table.
    mov $(pml4 - KERNEL_VMA), %eax
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
    lgdt p_gdt32 - KERNEL_VMA

    # Here we go bois.
    ljmp $gdt_code_seg, $(_start64 - KERNEL_VMA)

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

.code64
_start64:
    movabsq $(_start64_higherhalf - KERNEL_LMA), %rax
    jmp *%rax

_start64_higherhalf:
    mov $KERNEL_VMA, %rax
    add %rax, %rsp              # Adjust stack pointer to higher half

    # Invalidate paging
    // movq $0, (pml4)
    // invlpg 0

    call kernel_main

    cli
1:  hlt
    jmp 1b

boot64_err:
    # RSOD
    mov $0xdeadbeefcafebabe, %rax
    movl $0, %ecx
    movl $0x00ff0000, %edx       # ARGB
1:
    movl $0xfd000000, %edi       # framebuffer addr
    movl %edx, (%edi, %ecx, 4)   # draw argb[%edx] to framebuffer[%edi]
    inc %ecx
    cmpl $(SCREEN_WIDTH * SCREEN_HEIGHT), %ecx
    jne 1b
    hlt
