#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "kernel/paging.h"

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
#define PML4E_TO_ADDR(addr) (addr & 0xfffffffffffff000)
// PD or PT entry bits
#define PDE_HUGE (1 << 7)
#define PTE_PRESENT (1 << 0)
#define PTE_READWRITE (1 << 1)

uint64_t linear_limit = 0;

/**
 * Return a pointer to a PDPT, given a pointer to a PML4 entry.
 * If a PDPT does not yet exist for the PML4E, it will be allocated.
 */
uint64_t *paging_get_pdpt(uint64_t *pml4e)
{
    uint64_t *pdpt;
    if (*pml4e == 0) {
        pdpt = pmem_alloc(8 * 512);
    }
}

static void map_page(uint64_t physaddr, uint64_t virtaddr)
{
    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;
    if (pml4[PML4_INDEX(virtaddr)] == 0) {
        // Allocate and init an empty PDPT for this PML4E
        pdpt = pmem_alloc(8 * 512);
        for (size_t e = 0; e < 512; e++)
            pdpt[e] = 0;
        // Point this PML4E -> newly alloc'd PDPT
        pml4[PML4_INDEX(virtaddr)] =
            (uint64_t)pdpt | PTE_PRESENT | PTE_READWRITE;
    }
    pdpt = PML4E_TO_ADDR(pml4[PML4_INDEX(virtaddr)]);
    // Ensure PDPT entry has been allocated
    if (pdpt[PDPT_INDEX(virtaddr)] == 0) {
        // Allocate and init an empty PD for this PDPTE
        pd = pmem_alloc(8 * 512);
        for (size_t e = 0; e < 512; e++)
            pd[e] = 0;
        // Point this PDPTE -> newly alloc'd PD
        pdpt[PDPT_INDEX(virtaddr)] = (uint64_t)pd | PTE_PRESENT | PTE_READWRITE;
    }
    pd = PML4E_TO_ADDR(pdpt[PDPT_INDEX(virtaddr)]);
    // Ensure PD entry has been allocated
    if (pd[PD_INDEX(virtaddr)] == 0) {
        // Allocate and init an empty PT for this PDE
        pt = pmem_alloc(8 * 512);
        printf("pt addr = %x\n", pt);
        for (size_t e = 0; e < 512; e++)
            pt[e] = 0;
        // Point this PDE -> newly alloc'd PT
        pd[PD_INDEX(virtaddr)] = (uint64_t)pt | PTE_PRESENT | PTE_READWRITE;
    }
    pt = PML4E_TO_ADDR(pd[PD_INDEX(virtaddr)]);
    // printf("PT addr: %x ... idx: %u\n", pt, PT_INDEX(virtaddr));
    pt[PT_INDEX(virtaddr)] = physaddr | PTE_PRESENT | PTE_READWRITE;
}

static void map_hugepage(uint64_t physaddr, uint64_t virtaddr)
{
    uint64_t *pdpt;
    uint64_t *pd;
    // Ensure PML4 entry has been allocated
    if (pml4[PML4_INDEX(virtaddr)] == 0) {
        // Allocate and init an empty PDPT for this PML4E
        pdpt = pmem_alloc(8 * 512);
        for (size_t e = 0; e < 512; e++)
            pdpt[e] = 0;
        // Point this PML4E -> newly alloc'd PDPT
        pml4[PML4_INDEX(virtaddr)] =
            (uint64_t)pdpt | PTE_PRESENT | PTE_READWRITE;
    }
    pdpt = PML4E_TO_ADDR(pml4[PML4_INDEX(virtaddr)]);
    // Ensure PDPT entry has been allocated
    if (pdpt[PDPT_INDEX(virtaddr)] == 0) {
        // Allocate and init an empty PD for this PDPTE
        pd = pmem_alloc(8 * 512);
        for (size_t e = 0; e < 512; e++)
            pd[e] = 0;
        // Point this PDPTE -> newly alloc'd PD
        pdpt[PDPT_INDEX(virtaddr)] = (uint64_t)pd | PTE_PRESENT | PTE_READWRITE;
    }
    pd = PML4E_TO_ADDR(pdpt[PDPT_INDEX(virtaddr)]);
    pd[PD_INDEX(virtaddr)] = physaddr | PDE_HUGE | PTE_PRESENT | PTE_READWRITE;
}

/**
 * Setup paging:
 * mb2_info will NOT be available after this subroutine returns.
 */
void paging_init(struct mb2_info *mb2_info)
{
    // Zero-out the PML4
    for (size_t i = 0; i < 512; i++)
        pml4[i] = 0;

    /// Temp: Identity map lower 4G
    printf("Identity mapping lower 4G...\n");
    for (uint64_t physaddr = 0; physaddr < 0x100000000 /** 4G */;
         physaddr += HUGEPAGE_SIZE) {
        map_hugepage(physaddr, physaddr);
    }
    printf("Done.\n");

    /// Map bottom physical 2G to virtual -2G (lazy af) with 2M hugepages
    printf("Mapping kernel to upper half...\n");
    uint64_t upper_half_addr = KERNEL_VMA;
    for (uint64_t physaddr = 0; physaddr < 0x100000000 /** 4G */;
         physaddr += HUGEPAGE_SIZE, upper_half_addr += HUGEPAGE_SIZE) {
        map_hugepage(physaddr, upper_half_addr);
    }
    printf("Done.\n");

    // Recursive map PML4
    pml4[510] = (uint64_t)pml4 | PTE_PRESENT | PTE_READWRITE;

    // __asm__ volatile("hlt");
    /// Map the available RAM to a linear address space
    printf("Mapping linear address space...\n");
    for (size_t i = 0; i < pmem_count; i++) {
        struct pmem_block *block = pmem_map + i;
        printf("Mapping block starting at 0x%x -> 0x%x...\n", block->base,
               linear_limit);
        for (uint64_t physaddr = block->base; physaddr < block->limit;
             physaddr += PAGE_SIZE, linear_limit += PAGE_SIZE) {
            map_page(physaddr, linear_limit);
        }
    }
    printf("Done.\n");

    /// Map the linear framebuffer
    // __asm__ volatile("hlt");
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
         physaddr += HUGEPAGE_SIZE, virtaddr += HUGEPAGE_SIZE) {
        map_hugepage(physaddr, virtaddr);
    }
    printf("Done.\n");

    /// Reload CR3 with our new PML4 mapping
    // __asm__ volatile("hlt");
    __asm__ volatile(
        "movq %0, %%cr3" ::"r"((uint64_t)pml4 - 0xffffffff80000000));
}
