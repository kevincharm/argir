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
// Page table virtual addresses from recursive mapping @PML4[510]
#define PML4_ADDR ((uint64_t *)0xffffff7fbfdfe000ull)
#define PDPT_ADDR(linear_addr)                                                 \
    ((uint64_t *)(0xffffff7fbfc00000ull +                                      \
                  ((linear_addr >> (PAGE_OFFSET_BITS + PTE_BITS + PDE_BITS)) & \
                   0x1ff000ull)))
#define PD_ADDR(linear_addr)                                                   \
    ((uint64_t *)(0xffffff7f80000000ull +                                      \
                  ((linear_addr >> (PTE_BITS + PDE_BITS)) & 0x3ffff000ull)))
#define PT_ADDR(linear_addr)                                                   \
    ((uint64_t *)(0xffffff0000000000ull +                                      \
                  ((linear_addr >> (PDE_BITS)) & 0x7ffffff000ull)))
// Get page table entry pointers
#define PML4_ENTRY(linear_addr) PML4_ADDR[PML4_INDEX(linear_addr)]
#define PDPT_ENTRY(linear_addr) PDPT_ADDR(linear_addr)[PDPT_INDEX(linear_addr)]
#define PD_ENTRY(linear_addr) PD_ADDR(linear_addr)[PD_INDEX(linear_addr)]
#define PT_ENTRY(linear_addr) PT_ADDR(linear_addr)[PT_INDEX(linear_addr)]

// Get actual physical address from a PML4/PDPT/PD/PT entry (ignore lower 12-bits)
#define PML4E_TO_ADDR(addr) (addr & 0xfffffffffffff000)
// PD or PT entry bits
#define PDE_HUGE (1 << 7)
#define PTE_PRESENT (1 << 0)
#define PTE_READWRITE (1 << 1)

uint64_t linear_limit = 0;

/**
 * Invalidate TLB entry for `virtaddr`.
 */
static inline void invlpg(uint64_t virtaddr)
{
    __asm__ volatile("invlpg (%0)" ::"r"(virtaddr) : "memory");
}

/**
 * Map a 4K page starting at virtual memory address `virtaddr` to physical memory address `physaddr`.
 */
static void map_page(uint64_t virtaddr, uint64_t physaddr)
{
    uint64_t pdpt;
    uint64_t pd;
    uint64_t pt;
    if (PML4_ENTRY(virtaddr) & PTE_PRESENT != 0x1) {
        // Allocate and init an empty PDPT for this PML4E
        pdpt = pmem_alloc_page();
        // printf("Mapped PML4[%x] <- %x\n", PML4_INDEX(virtaddr), pdpt);
        // Point this PML4E -> newly alloc'd PDPT
        PML4_ENTRY(virtaddr) = pdpt | PTE_PRESENT | PTE_READWRITE;
        // invlpg(PDPT_ADDR(virtaddr));
    }
    // Ensure PDPT entry has been allocated
    if (PDPT_ENTRY(virtaddr) & PTE_PRESENT != 0x1) {
        // Allocate and init an empty PD for this PDPTE
        pd = pmem_alloc_page();
        // printf("Mapped PDPT[%x] <- %x\n", PDPT_INDEX(virtaddr), pd);
        // Point this PDPTE -> newly alloc'd PD
        PDPT_ENTRY(virtaddr) = pd | PTE_PRESENT | PTE_READWRITE;
        // invlpg(PD_ADDR(virtaddr));
    }
    // Ensure PD entry has been allocated
    if (PD_ENTRY(virtaddr) & PTE_PRESENT != 0x1) {
        // Allocate and init an empty PT for this PDE
        pt = pmem_alloc_page();
        // printf("Mapped PT[%x] <- %x\n", PD_INDEX(virtaddr), pt);
        // Point this PDE -> newly alloc'd PT
        PD_ENTRY(virtaddr) = pt | PTE_PRESENT | PTE_READWRITE;
        // invlpg(PT_ADDR(virtaddr));
    }
    PT_ENTRY(virtaddr) =
        (physaddr & 0xffffffffff000ull) | PTE_PRESENT | PTE_READWRITE;
}

static const uint64_t temp_virtaddr =
    0xffffffff801fd000ull; // Within the first 2M of kernel space (bootloader)

/**
 * Temporarily map a physical address `physaddr` so that it is accessible.
 */
void *map_temp_page(uint64_t physaddr)
{
    PT_ENTRY(temp_virtaddr) = physaddr | PTE_PRESENT | PTE_READWRITE;
    invlpg(temp_virtaddr);
    return temp_virtaddr;
}

/**
 * Unmap the last temporarily-mapped physical address.
 */
void unmap_temp_page()
{
    PT_ENTRY(temp_virtaddr) = 0;
    invlpg(temp_virtaddr);
}

/**
 * Setup paging:
 * mb2_info will NOT be available after this subroutine returns.
 */
void paging_init(struct mb2_info *mb2_info)
{
    /// Temp: Identity map lower 4G
    // printf("Identity mapping lower 4G...\n");
    // for (uint64_t physaddr = 0; physaddr < 0x100000000 /** 4G */;
    //      physaddr += HUGEPAGE_SIZE) {
    //     map_hugepage(physaddr, physaddr);
    // }
    // printf("Done.\n");

    /// Map bottom physical 2G to virtual -2G (lazy af) with 2M hugepages
    // printf("Mapping kernel to upper half...\n");
    // uint64_t upper_half_addr = KERNEL_VMA;
    // for (uint64_t physaddr = 0; physaddr < 0x100000000 /** 4G */;
    //      physaddr += HUGEPAGE_SIZE, upper_half_addr += HUGEPAGE_SIZE) {
    //     map_hugepage(upper_half_addr, physaddr);
    // }
    // printf("Done.\n");

    /// Map the available RAM to a linear address space
    printf("Mapping linear address space...\n");
    for (size_t i = 0; i < pmem_blocks_count; i++) {
        struct pmem_block *block = pmem_block_map + i;
        printf("Mapping block starting at 0x%x -> [0x%x, 0x%x) (%u bytes)\n",
               linear_limit, block->base, block->limit,
               block->limit - block->base);
        for (uint64_t physaddr = block->base; physaddr < block->limit;
             physaddr += PAGE_SIZE, linear_limit += PAGE_SIZE) {
            /// DEBUG: We are overwriting the framebuffer at some point
            // somewhere between 0x14000000 and 0x15000000
            // if (physaddr >= 0x14000000 && physaddr < 0x100000000) {
            //     printf("Skip map 0x%x -> 0x%x...\n", linear_limit, physaddr);
            //     break;
            // }
            map_page(linear_limit, physaddr);
        }
    }
    printf("Done.\n");

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

    /// Reload CR3 with our new PML4 mapping
    // __asm__ volatile("hlt");
    uint64_t pml4;
    __asm__ volatile("movq %%cr3, %0" : "=r"(pml4));
    __asm__ volatile("movq %0, %%cr3" ::"r"(pml4) : "memory");
}
