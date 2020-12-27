#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "kernel/addr.h"
#include "kernel/paging.h"
#include "kernel/terminal.h"

/// Page table utils
// Offsets
#define PAGE_OFFSET_BITS (12)
#define PTE_BITS (9)
#define PDE_BITS (9)
#define PDPTE_BITS (9)
#define PML4E_BITS (9)
// Page table index calculators
#define PML4_INDEX(linear_addr)                                                \
    ((linear_addr >> (PAGE_OFFSET_BITS + PTE_BITS + PDE_BITS + PDPTE_BITS)) &  \
     0x1ff)
#define PDPT_INDEX(linear_addr)                                                \
    ((linear_addr >> (PAGE_OFFSET_BITS + PTE_BITS + PDE_BITS)) & 0x1ff)
#define PD_INDEX(linear_addr)                                                  \
    ((linear_addr >> (PAGE_OFFSET_BITS + PTE_BITS)) & 0x1ff)
#define PT_INDEX(linear_addr) ((linear_addr >> PAGE_OFFSET_BITS) & 0x1ff)
#define PAGE_OFF(linear_addr) ((linear_addr & 0xfff))
// Get actual physical address from a PML4/PDPT/PD/PT entry (ignore lower 12-bits)
#define PML4E_TO_ADDR(addr) (addr & 0xfffffffffffff000ull)
#define PML4E_TO_HIGH_ADDR(addr) (PML4E_TO_ADDR(addr) + KERNEL_VMA)
// PD or PT entry bits
#define PDE_HUGE (1 << 7)
#define PTE_PRESENT (1 << 0)
#define PTE_READWRITE (1 << 1)

uint64_t kernel_pml4[512] __attribute__((aligned(PAGE_SIZE)));
uint64_t kernel_pdpt0[512] __attribute__((aligned(PAGE_SIZE)));
uint64_t kernel_pd0[512]
    __attribute__((aligned(PAGE_SIZE))); // [0, 1G); kernel code
uint64_t kernel_pd1[512]
    __attribute__((aligned(PAGE_SIZE))); // [1G, 2G); kernel code
uint64_t kernel_pd2[512]
    __attribute__((aligned(PAGE_SIZE))); // [2G, 3G); TODO: still needed?
uint64_t kernel_pd3[512]
    __attribute__((aligned(PAGE_SIZE))); // [3G, 4G); TODO: still needed?

uint64_t linear_limit = 0;

/**
 * Invalidate TLB entry for `virtaddr`.
 */
static inline void invlpg(uint64_t virtaddr)
{
    __asm__ volatile("invlpg (%0)" ::"r"(virtaddr) : "memory");
}

#define TEMP_MAP_ADDR (0xffffffffffe00000ull) // -2M, mapped as part of kernel

static void *paging_temp_map(uint64_t physaddr)
{
    // Align physaddr down to a 2M boundary (kernel is mapped to hugepages)
    uint64_t rem = physaddr % 0x200000;
    physaddr -= rem;

    kernel_pd1[511] =
        physaddr | PDE_HUGE | PTE_PRESENT |
        PTE_READWRITE; // virtual -2M maps to last entry in PML4_511->PDPT_511->PD_1

    uint64_t virtaddr = TEMP_MAP_ADDR + rem;
    invlpg(virtaddr);
    return virtaddr;
}

static void paging_unmap_temp()
{
    kernel_pd1[511] = 0;
    invlpg(TEMP_MAP_ADDR);
}

/**
 * Map a 4K page starting at virtual memory address `virtaddr` to physical memory address `physaddr`.
 */
static void map_page(uint64_t virtaddr, uint64_t physaddr)
{
    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;
    if (!(kernel_pml4[PML4_INDEX(virtaddr)] & PTE_PRESENT)) {
        // Allocate and init an empty PDPT for this PML4E
        pdpt = pmem_alloc_page();
        // Point this PML4E -> newly alloc'd PDPT
        kernel_pml4[PML4_INDEX(virtaddr)] =
            (uint64_t)pdpt | PTE_PRESENT | PTE_READWRITE;
        // Map the pdpt to a virt addr so we can access it
        uint64_t *v_pdpt = paging_temp_map(pdpt);
        for (size_t e = 0; e < 512; e++)
            v_pdpt[e] = 0;
    }
    pdpt = PML4E_TO_HIGH_ADDR(kernel_pml4[PML4_INDEX(virtaddr)]);
    /// DEBUG: Temporarily map this address so that it is accessible
    // Ensure PDPT entry has been allocated
    if (!(pdpt[PDPT_INDEX(virtaddr)] & PTE_PRESENT)) {
        // Allocate and init an empty PD for this PDPTE
        pd = pmem_alloc_page();
        // Point this PDPTE -> newly alloc'd PD
        pdpt[PDPT_INDEX(virtaddr)] = (uint64_t)pd | PTE_PRESENT | PTE_READWRITE;
        // Map the pd to a virt addr so we can access it
        uint64_t *v_pd = paging_temp_map(pd);
        for (size_t e = 0; e < 512; e++)
            v_pd[e] = 0;
    }
    pd = PML4E_TO_HIGH_ADDR(pdpt[PDPT_INDEX(virtaddr)]);
    // Ensure PD entry has been allocated
    if (!(pd[PD_INDEX(virtaddr)] & PTE_PRESENT)) {
        // Allocate and init an empty PT for this PDE
        pt = pmem_alloc_page();
        // Point this PDE -> newly alloc'd PT
        pd[PD_INDEX(virtaddr)] = (uint64_t)pt | PTE_PRESENT | PTE_READWRITE;
        // Map the pd to a virt addr so we can access it
        uint64_t *v_pt = paging_temp_map(pt);
        for (size_t e = 0; e < 512; e++)
            v_pt[e] = 0;
    }
    pt = PML4E_TO_HIGH_ADDR(pd[PD_INDEX(virtaddr)]);
    // printf("PT addr: %x ... idx: %u\n", pt, PT_INDEX(virtaddr));
    pt[PT_INDEX(virtaddr)] = physaddr | PTE_PRESENT | PTE_READWRITE;
}

static void paging_flush_tlb()
{
    uint64_t pml4;
    __asm__ volatile("movq %%cr3, %0" : "=r"(pml4));
    __asm__ volatile("movq %0, %%cr3" ::"r"(pml4) : "memory");
}

/**
 * Remap LFB from lower-half address to higher-half address (-3G)
 */
static void paging_remap_lfb(struct mb2_info *mb2_info)
{
    /// Map the linear framebuffer
    printf("Remapping framebuffer...\n");
    struct mb2_tag *tag_fb = mb2_find_tag(mb2_info, MB_TAG_TYPE_FRAMEBUFFER);
    if (tag_fb == NULL) {
        // TODO: Panic
        printf("Failed to find framebuffer tag in boot info.\n");
        return;
    }

    // Bytes, pages needed for entire framebuffer
    size_t fb_size = tag_fb->framebuffer.pitch * tag_fb->framebuffer.height;
    if (fb_size > 0x40000000 /** 1G */) {
        // We assume that the entire LFB fits inside 1GiB
        // TODO: Panic
        printf("LFB is larger than 1GiB!\n");
        return;
    }

    // We'll map the LFB to -3G in virtual mem (1G below kernel)
    uint64_t virtaddr = LFB_VMA;
    for (uint64_t physaddr = tag_fb->framebuffer.addr;
         physaddr < tag_fb->framebuffer.addr + fb_size;
         physaddr += PAGE_SIZE, virtaddr += PAGE_SIZE) {
        map_page(virtaddr, physaddr);
    }
    printf("Done.\n");
}

/**
 * Setup paging:
 * mb2_info will NOT be available after this subroutine returns.
 */
void paging_init(struct mb2_info *mb2_info)
{
    // Drop identity (0-4G), remap LFB to -3G
    // uint64_t *test = TEMP_MAP_ADDR;
    // uint64_t *test = paging_temp_map(0x7fd00000);
    // uint64_t *test = kernel_pd1 + 511;
    // __asm__ volatile("mov %0, %%rax\n\t"
    //                  "1: jmp 1b" ::"r"(*test));
    paging_remap_lfb(mb2_info);
    paging_flush_tlb();
    __asm__ volatile("1: jmp 1b");

    // Re-initialise LFB with higher-half address
    struct mb2_tag *tag_fb = mb2_find_tag(mb2_info, MB_TAG_TYPE_FRAMEBUFFER);
    size_t width = tag_fb->framebuffer.width;
    size_t height = tag_fb->framebuffer.height;
    size_t pitch = tag_fb->framebuffer.pitch;
    terminal_init(LFB_VMA, width, height, pitch);

    /// Map the available RAM to a linear address space
    printf("Mapping linear address space...\n");
    for (size_t i = 0; i < pmem_blocks_count; i++) {
        struct pmem_block *block = pmem_block_map + i;
        printf("Mapping block starting at 0x%x -> [0x%x, 0x%x) (%u bytes)\n",
               linear_limit, block->base, block->limit,
               block->limit - block->base);
        for (uint64_t physaddr = block->base; physaddr < block->limit;
             physaddr += PAGE_SIZE, linear_limit += PAGE_SIZE) {
            map_page(linear_limit, physaddr);
        }
    }
    printf("Done.\n");

    /// Reload CR3 with our new PML4 mapping
    paging_flush_tlb();
}
