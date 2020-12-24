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
    ((linear_addr >> (PAGE_OFFSET_BITS + PTE_BITS + PDE_BITS)) & 0x3ffff)
#define PD_INDEX(linear_addr)                                                  \
    ((linear_addr >> (PAGE_OFFSET_BITS + PTE_BITS)) & 0x7ffffff)
#define PT_INDEX(linear_addr)                                                  \
    ((linear_addr >> PAGE_OFFSET_BITS) & 0xffffffffful)

// Page table virtual addresses from recursive mapping @PML4[510]
#define PML4_ADDR ((uint64_t *)0xffffff7fbfdfe000ull)
#define PDPT_ADDR ((uint64_t *)0xffffff7fbfc00000ull)
#define PD_ADDR ((uint64_t *)0xffffff7f80000000ull)
#define PT_ADDR ((uint64_t *)0xffffff0000000000ull)
// Get page table entry pointers
#define PML4_ENTRY(linear_addr) PML4_ADDR[PML4_INDEX(linear_addr)]
#define PDPT_ENTRY(linear_addr) PDPT_ADDR[PDPT_INDEX(linear_addr)]
#define PD_ENTRY(linear_addr) PD_ADDR[PD_INDEX(linear_addr)]
#define PT_ENTRY(linear_addr) PT_ADDR[PT_INDEX(linear_addr)]

// Get actual physical address from a PML4/PDPT/PD/PT entry (ignore lower 12-bits)
#define PML4E_TO_ADDR(addr) (addr & 0xfffffffffffff000)
// PD or PT entry bits
#define PDE_HUGE (1 << 7)
#define PTE_PRESENT (1 << 0)
#define PTE_READWRITE (1 << 1)

uint64_t kernel_pml4[512] __attribute__((aligned(PAGE_SIZE)));

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
    if (!(PML4_ENTRY(virtaddr) & PTE_PRESENT)) {
        // Allocate and init an empty PDPT for this PML4E
        pdpt = pmem_alloc_page();
        // printf("Mapped PML4[%x] <- %x\n", PML4_INDEX(virtaddr), pdpt);
        // Point this PML4E -> newly alloc'd PDPT
        PML4_ENTRY(virtaddr) = pdpt | PTE_PRESENT | PTE_READWRITE;
        invlpg(PDPT_ENTRY(virtaddr));
    }
    // Ensure PDPT entry has been allocated
    if (!(PDPT_ENTRY(virtaddr) & PTE_PRESENT)) {
        // Allocate and init an empty PD for this PDPTE
        pd = pmem_alloc_page();
        // printf("Mapped PDPT[%x] <- %x\n", PDPT_INDEX(virtaddr), pd);
        // Point this PDPTE -> newly alloc'd PD
        PDPT_ENTRY(virtaddr) = pd | PTE_PRESENT | PTE_READWRITE;
        invlpg(PD_ENTRY(virtaddr));
    }
    // Ensure PD entry has been allocated
    if (!(PD_ENTRY(virtaddr) & PTE_PRESENT)) {
        // Allocate and init an empty PT for this PDE
        pt = pmem_alloc_page();
        // printf("Mapped PD[%x] <- %x\n", PD_INDEX(virtaddr), pt);
        // Point this PDE -> newly alloc'd PT
        PD_ENTRY(virtaddr) = pt | PTE_PRESENT | PTE_READWRITE;
        invlpg(PT_ENTRY(virtaddr));
    }
    if (PT_ENTRY(virtaddr) & PTE_PRESENT) {
        printf("--- FATAL ---\n");
        printf("PML4[%u] = 0x%x\n", PML4_INDEX(virtaddr), PML4_ENTRY(virtaddr));
        printf("PDPT[%u] = 0x%x\n", PDPT_INDEX(virtaddr), PDPT_ENTRY(virtaddr));
        printf("PD[%u] = 0x%x\n", PD_INDEX(virtaddr), PD_ENTRY(virtaddr));
        printf("PT[%u] = 0x%x\n", PT_INDEX(virtaddr), PT_ENTRY(virtaddr));
        printf("FATAL: Entry for 0x%x is already occupied!\n\n", virtaddr);
        __asm__ volatile("mov $0xdeadbeefdeadbeef, %rax\n\t"
                         "1: jmp 1b");
        return;
    }

    // printf("SET 0x%x -> PT[%u] = 0x%x\n\n", virtaddr, PT_INDEX(virtaddr),
    //        physaddr);
    PT_ENTRY(virtaddr) =
        (physaddr & 0xffffffffff000ull) | PTE_PRESENT | PTE_READWRITE;
    __asm__ volatile("mov %0, %%rax\n\t" ::"m"(virtaddr));
    invlpg(virtaddr);
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
    printf("PML4[0] = 0x%x\n", PML4_ENTRY(0));
    /// DEBUG: This should unmap the kernel from -2G and cause a triple fault, but it doesn't!
    PML4_ENTRY(KERNEL_VMA) = 0;
    /// DEBUG: When ref'd directly, it does cause a crash.
    // kernel_pml4[511] = 0;
    paging_flush_tlb();
    // __asm__ volatile("mov %0, %%rax\n\t"
    //                  "1: jmp 1b" ::"m"(p));
    __asm__ volatile("1: jmp 1b");
    paging_remap_lfb(mb2_info);
    paging_flush_tlb();

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
