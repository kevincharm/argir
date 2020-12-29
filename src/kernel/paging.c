#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <memory.h>
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

#define TO_LOWER_HALF(virtaddr) ((uint64_t)virtaddr - KERNEL_VMA)

uint64_t kernel_pml4[512] __attribute__((aligned(PAGE_SIZE)));
uint64_t kernel_pdpt0[512] __attribute__((aligned(PAGE_SIZE)));
/// -2G
uint64_t kernel_pd0[512] __attribute__((aligned(PAGE_SIZE)));
uint64_t kernel_pd0_pt[512 * 512]
    __attribute__((aligned(PAGE_SIZE))); // [0, 1G); kernel code
uint64_t kernel_pd1[512] __attribute__((aligned(PAGE_SIZE)));
uint64_t kernel_pd1_pt[512 * 512]
    __attribute__((aligned(PAGE_SIZE))); // [1G, 2G); kernel code

uint64_t linear_limit = 0;

/**
 * Invalidate TLB entry for `virtaddr`.
 */
static inline void invlpg(uint64_t virtaddr)
{
    __asm__ volatile("invlpg (%0)" ::"r"(virtaddr) : "memory");
}

/**
 * Reload PML4 in CR3.
 */
static void paging_flush_tlb()
{
    uint64_t pml4;
    __asm__ volatile("movq %%cr3, %0" : "=r"(pml4));
    __asm__ volatile("movq %0, %%cr3" ::"r"(pml4) : "memory");
}

#define TEMP_MAP_ADDR (KERNEL_VMA) // Reclaim first 4K, mapped as part of kernel

static void paging_unmap_temp()
{
    kernel_pd0_pt[0] = 0;
    invlpg(TEMP_MAP_ADDR);
}

/**
 * Map 4K page at TEMP_MAP_ADDR
 */
static void *paging_temp_map(uint64_t physaddr)
{
    paging_unmap_temp();

    kernel_pd0_pt[0] = PML4E_TO_ADDR(physaddr) | PTE_PRESENT |
                       PTE_READWRITE; // first 4K page of kernel
    invlpg(TEMP_MAP_ADDR);
    return TEMP_MAP_ADDR;
}

/**
 * Map a 4K page starting at virtual memory address `virtaddr` to physical memory address `physaddr`.
 * NOTE: Doesn't invalidate pages or flush TLB, so do it yourself.
 */
static void paging_map_page(uint64_t virtaddr, uint64_t physaddr)
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
        memset(v_pdpt, 0, 512 * (sizeof *v_pdpt));
    }
    pdpt = paging_temp_map(kernel_pml4[PML4_INDEX(virtaddr)]);
    if (!(pdpt[PDPT_INDEX(virtaddr)] & PTE_PRESENT)) {
        // Allocate and init an empty PD for this PDPTE
        pd = pmem_alloc_page();
        // Point this PDPTE -> newly alloc'd PD
        pdpt[PDPT_INDEX(virtaddr)] = (uint64_t)pd | PTE_PRESENT | PTE_READWRITE;
        // Map the pd to a virt addr so we can access it
        uint64_t *v_pd = paging_temp_map(pd);
        memset(v_pd, 0, 512 * (sizeof *v_pd));
        // Remap the PDPT as we will need to access it again
        pdpt = paging_temp_map(kernel_pml4[PML4_INDEX(virtaddr)]);
    }
    pd = paging_temp_map(pdpt[PDPT_INDEX(virtaddr)]);
    // Ensure PD entry has been allocated
    if (!(pd[PD_INDEX(virtaddr)] & PTE_PRESENT)) {
        if (pd[PD_INDEX(virtaddr)] & PDE_HUGE) {
            // Fatal? Trying to map a 4K page where a hugepage (2M) is already mapped
            __asm__ volatile("mov $0xbaaaaaadbeeeeeef, %rax\n\t"
                             "1: jmp 1b");
        }
        // Allocate and init an empty PT for this PDE
        pt = pmem_alloc_page();
        // Point this PDE -> newly alloc'd PT
        pd[PD_INDEX(virtaddr)] = (uint64_t)pt | PTE_PRESENT | PTE_READWRITE;
        // Map the pd to a virt addr so we can access it
        uint64_t *v_pt = paging_temp_map(pt);
        memset(v_pt, 0, 512 * (sizeof *v_pt));
        // Remap the PD as we will need to access it again
        // This is a bit convoluted: PDPT is no longer available,
        // so we have to consecutively map it from a known mapped address (PML4).
        pdpt = paging_temp_map(kernel_pml4[PML4_INDEX(virtaddr)]);
        pd = paging_temp_map(pdpt[PDPT_INDEX(virtaddr)]);
    }
    pt = paging_temp_map(pd[PD_INDEX(virtaddr)]);
    pt[PT_INDEX(virtaddr)] = physaddr | PTE_PRESENT | PTE_READWRITE;
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
        paging_map_page(virtaddr, physaddr);
    }
    printf("Done.\n");
}

/**
 * Remap 2G of kernel code to *4K pages* at higher-half starting at -2G.
 */
static void paging_remap_kernel()
{
    memset(kernel_pml4, 0, 8 * 512);
    memset(kernel_pdpt0, 0, 8 * 512);
    memset(kernel_pd0, 0, 8 * 512);
    memset(kernel_pd0_pt, 0, 8 * 512 * 512);
    memset(kernel_pd1, 0, 8 * 512);
    memset(kernel_pd1_pt, 0, 8 * 512 * 512);

    // Map virtual higher-half addresses starting at -2G to physical addresses [0G, 2G)
    kernel_pml4[511] =
        TO_LOWER_HALF((uint64_t)kernel_pdpt0) | PTE_PRESENT | PTE_READWRITE;
    kernel_pdpt0[510] =
        TO_LOWER_HALF((uint64_t)kernel_pd0) | PTE_PRESENT | PTE_READWRITE;
    uint64_t physaddr = 0;
    // [0G, 1G)
    for (size_t i = 0; i < 512; i++) {
        kernel_pd0[i] = TO_LOWER_HALF((uint64_t)(&(kernel_pd0_pt[i * 512]))) |
                        PTE_PRESENT | PTE_READWRITE;
        for (size_t j = 0; j < 512; j++, physaddr += 0x1000) {
            kernel_pd0_pt[i * 512 + j] = physaddr | PTE_PRESENT | PTE_READWRITE;
        }
    }
    // [1G, 2G)
    for (size_t i = 0; i < 512; i++) {
        kernel_pd1[i] = TO_LOWER_HALF((uint64_t)(&(kernel_pd1_pt[i * 512]))) |
                        PTE_PRESENT | PTE_READWRITE;
        for (size_t j = 0; j < 512; j++, physaddr += 0x1000) {
            kernel_pd1_pt[i * 512 + j] = physaddr | PTE_PRESENT | PTE_READWRITE;
        }
    }

    // Replace the boot PML4
    uint64_t pml4 = (uint64_t)kernel_pml4 - KERNEL_VMA; // physaddr of new PML4
    __asm__ volatile("movq %0, %%cr3" ::"r"(pml4) : "memory");
}

/**
 * Setup paging:
 *  - Remap kernel to 4K pages
 *  - Remap LFB to higher-half
 *  - Re-initialise terminal with new LFB
 *  - Map bottom-half linear address space to available physical space from memory map
 */
void paging_init(struct mb2_info *mb2_info)
{
    paging_remap_kernel();
    paging_remap_lfb(mb2_info);
    paging_flush_tlb();

    // Re-initialise LFB with higher-half address
    struct mb2_tag *tag_fb = mb2_find_tag(mb2_info, MB_TAG_TYPE_FRAMEBUFFER);
    size_t width = tag_fb->framebuffer.width;
    size_t height = tag_fb->framebuffer.height;
    size_t pitch = tag_fb->framebuffer.pitch;
    terminal_init(LFB_VMA, width, height, pitch, 2);

    /// Map the available RAM to a linear address space
    printf("Mapping linear address space...\n");
    for (size_t i = 0; i < pmem_blocks_count; i++) {
        struct pmem_block *block = pmem_block_map + i;
        printf("Mapping block starting at 0x%x -> [0x%x, 0x%x) (%u bytes)\n",
               linear_limit, block->base, block->limit,
               block->limit - block->base);
        for (uint64_t physaddr = block->base; physaddr < block->limit;
             physaddr += PAGE_SIZE, linear_limit += PAGE_SIZE) {
            paging_map_page(linear_limit, physaddr);
        }
    }
    printf("Done.\n");

    /// Reload CR3 with our new PML4 mapping
    paging_flush_tlb();
    terminal_clear();
}
