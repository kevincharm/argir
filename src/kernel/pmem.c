#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <memory.h>
#include <algo.h>
#include <kernel/mb2.h>

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

#define PAGE_SIZE (0x1000) /** 4K */
#define HUGEPAGE_SIZE (0x200000) /** 2M */

struct pmem_block {
    uint64_t base;
    uint64_t limit;
};

// 1GiB / 4K = 262,144 pages
// 262,144 pages = 256K bitmap space per GiB
#define PMEM_MAP_MAX_POOL (16 * 262144) /** 4M pool -> 16G addressable */
/// Early static physical alloc map
uint8_t pmem_alloc_map[PMEM_MAP_MAX_POOL];
#define PMEM_ALLOC_MAP_USED (1 << 0)
#define PMEM_ALLOC_MAP_UNINITIALISED (1 << 7)

#define MAX_PMEM_ENTRIES (128) /** 128 * 16B = 2K, enough for now? */
/// Memory map from MB2 boot info, sorted and merged
struct pmem_block pmem_map[MAX_PMEM_ENTRIES];
size_t pmem_count = 0;

// PML4 is allocated in `paging_init`
uint64_t pml4[512] __attribute__((aligned(4096)));
// Limit of the linear address space after `paging_init`
uint64_t linear_limit = 0;

void *pmem_alloc(size_t bytes)
{
    if (bytes == 0) {
        // wat
        // TODO: Panic
        printf("Could not allocate 0 bytes from physical memory!\n");
        return 0xbaadbeef;
    }

    // Pages needed == clear bits needed
    size_t pages = bytes / PAGE_SIZE;
    if (bytes % PAGE_SIZE != 0) {
        pages += 1;
    }

    // Find a free slice, mark it as allocated
    bool found = false;
    size_t byte_off = 0;
    size_t begin_free = 0;
    do {
        size_t pages_rem = pages;
        // Search for an unallocated page
        for (; byte_off < PMEM_MAP_MAX_POOL; byte_off++) {
            if (!(pmem_alloc_map[byte_off] & PMEM_ALLOC_MAP_USED)) {
                // Available
                begin_free = byte_off;
                break;
            }
        }

        // Check for a large-enough contiguous block
        for (byte_off = begin_free + 1; byte_off < PMEM_MAP_MAX_POOL;
             byte_off++) {
            if (pmem_alloc_map[byte_off] & PMEM_ALLOC_MAP_USED) {
                // Used, start again
                break;
            }

            pages_rem -= 1;

            if (pages_rem == 0) {
                // Current block [begin_free, byte_off] is eligible
                found = true;
                break;
            }
        }

        if (byte_off >= PMEM_MAP_MAX_POOL)
            break;
    } while (!found);

    if (!found) {
        // TODO: Panic
        printf("Could not allocate %u bytes from physical memory!\n", bytes);
        return NULL;
    }

    // Mark slice as allocated
    for (size_t i = begin_free; i < byte_off; i++) {
        pmem_alloc_map[i] &= ~PMEM_ALLOC_MAP_UNINITIALISED;
        pmem_alloc_map[i] |= PMEM_ALLOC_MAP_USED;
    }

    return begin_free * PAGE_SIZE;
}

/**
 * Mark all pages in a block of contiguous physical memory as free.
 */
void pmem_free(uint64_t base, uint64_t limit)
{
    // Mark free pages contained in this contiguous block of RAM
    uint64_t page_off;
    size_t byte_off;
    for (page_off = base; page_off < limit; page_off += PAGE_SIZE) {
        byte_off = page_off / PAGE_SIZE;
        if (byte_off >= PMEM_MAP_MAX_POOL) {
            // TODO: Panic
            printf("Ran out of bitmap memory pool! (byte_off = %u)\n",
                   byte_off);
            return;
        }
        *(pmem_alloc_map + byte_off) = 0;
    }
}

void paging_init(struct mb2_info *mb2_info)
{
    // Zero-out the PML4
    for (size_t i = 0; i < 512; i++)
        pml4[i] = 0;

    /// Temp: Identity map lower 4G
    printf("Identity mapping lower 4G...\n");
    for (uint64_t physaddr = 0; physaddr < 0x100000000;
         physaddr += HUGEPAGE_SIZE) {
        uint64_t *pdpt;
        uint64_t *pd;
        uint64_t *pt;
        // Ensure PML4 entry has been allocated
        if (pml4[PML4_INDEX(physaddr)] == 0) {
            // Allocate and init an empty PDPT for this PML4E
            pdpt = pmem_alloc(8 * 512);
            for (size_t e = 0; e < 512; e++)
                pdpt[e] = 0;
            // Point this PML4E -> newly alloc'd PDPT
            pml4[PML4_INDEX(physaddr)] =
                (uint64_t)pdpt | PTE_PRESENT | PTE_READWRITE;
        }
        pdpt = PML4E_TO_ADDR(pml4[PML4_INDEX(physaddr)]);
        // Ensure PDPT entry has been allocated
        if (pdpt[PDPT_INDEX(physaddr)] == 0) {
            // Allocate and init an empty PD for this PDPTE
            pd = pmem_alloc(8 * 512);
            for (size_t e = 0; e < 512; e++)
                pd[e] = 0;
            // Point this PDPTE -> newly alloc'd PD
            pdpt[PDPT_INDEX(physaddr)] =
                (uint64_t)pd | PTE_PRESENT | PTE_READWRITE;
        }
        pd = PML4E_TO_ADDR(pdpt[PDPT_INDEX(physaddr)]);
        pd[PD_INDEX(physaddr)] =
            physaddr | PDE_HUGE | PTE_PRESENT | PTE_READWRITE;
    }
    printf("Done.\n");

    /// Map bottom physical 2G to virtual -2G (lazy af) with 2M hugepages
    printf("Mapping kernel to upper half...\n");
    uint64_t upper_half_addr = 0xffffffff80000000;
    for (uint64_t physaddr = 0; physaddr < 0x100000000;
         physaddr += HUGEPAGE_SIZE, upper_half_addr += HUGEPAGE_SIZE) {
        uint64_t *pdpt;
        uint64_t *pd;
        uint64_t *pt;
        // Ensure PML4 entry has been allocated
        if (pml4[PML4_INDEX(upper_half_addr)] == 0) {
            // Allocate and init an empty PDPT for this PML4E
            pdpt = pmem_alloc(8 * 512);
            for (size_t e = 0; e < 512; e++)
                pdpt[e] = 0;
            // Point this PML4E -> newly alloc'd PDPT
            pml4[PML4_INDEX(upper_half_addr)] =
                (uint64_t)pdpt | PTE_PRESENT | PTE_READWRITE;
        }
        pdpt = PML4E_TO_ADDR(pml4[PML4_INDEX(upper_half_addr)]);
        // Ensure PDPT entry has been allocated
        if (pdpt[PDPT_INDEX(upper_half_addr)] == 0) {
            // Allocate and init an empty PD for this PDPTE
            pd = pmem_alloc(8 * 512);
            for (size_t e = 0; e < 512; e++)
                pd[e] = 0;
            // Point this PDPTE -> newly alloc'd PD
            pdpt[PDPT_INDEX(upper_half_addr)] =
                (uint64_t)pd | PTE_PRESENT | PTE_READWRITE;
        }
        pd = PML4E_TO_ADDR(pdpt[PDPT_INDEX(upper_half_addr)]);
        pd[PD_INDEX(upper_half_addr)] =
            physaddr | PDE_HUGE | PTE_PRESENT | PTE_READWRITE;
        // printf("Wrote PDE: 0x%x\n", pd[PD_INDEX(upper_half_addr)]);
    }
    printf("Done.\n");

    /// Map the available RAM to a linear address space
    printf("Mapping linear address space...\n");
    for (size_t i = 0; i < pmem_count; i++) {
        struct pmem_block *block = pmem_map + i;
        printf("Mapping block starting at 0x%x -> 0x%x...\n", block->base,
               linear_limit);
        for (uint64_t physaddr = block->base; physaddr < block->limit;

             physaddr += PAGE_SIZE, linear_limit += PAGE_SIZE) {
            uint64_t *pdpt;
            uint64_t *pd;
            uint64_t *pt;
            if (pml4[PML4_INDEX(linear_limit)] == 0) {
                // Allocate and init an empty PDPT for this PML4E
                pdpt = pmem_alloc(8 * 512);
                for (size_t e = 0; e < 512; e++)
                    pdpt[e] = 0;
                // Point this PML4E -> newly alloc'd PDPT
                pml4[PML4_INDEX(linear_limit)] =
                    (uint64_t)pdpt | PTE_PRESENT | PTE_READWRITE;
            }
            pdpt = PML4E_TO_ADDR(pml4[PML4_INDEX(linear_limit)]);
            // Ensure PDPT entry has been allocated
            if (pdpt[PDPT_INDEX(linear_limit)] == 0) {
                // Allocate and init an empty PD for this PDPTE
                pd = pmem_alloc(8 * 512);
                for (size_t e = 0; e < 512; e++)
                    pd[e] = 0;
                // Point this PDPTE -> newly alloc'd PD
                pdpt[PDPT_INDEX(linear_limit)] =
                    (uint64_t)pd | PTE_PRESENT | PTE_READWRITE;
            }
            pd = PML4E_TO_ADDR(pdpt[PDPT_INDEX(linear_limit)]);
            // Ensure PD entry has been allocated
            if (pd[PD_INDEX(linear_limit)] == 0) {
                // Allocate and init an empty PT for this PDE
                pt = pmem_alloc(8 * 512);
                for (size_t e = 0; e < 512; e++)
                    pt[e] = 0;
                // Point this PDE -> newly alloc'd PT
                pd[PD_INDEX(linear_limit)] =
                    (uint64_t)pt | PTE_PRESENT | PTE_READWRITE;
            }
            pt = PML4E_TO_ADDR(pd[PD_INDEX(linear_limit)]);
            pt[PT_INDEX(linear_limit)] = physaddr | PTE_PRESENT | PTE_READWRITE;
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
    if (fb_size > 0x40000000) {
        // We assume that the entire LFB fits inside 1GB
        // TODO: Panic
        printf("LFB is larger than 1GiB!\n");
        return;
    }

    // We'll map the LFB to -3G in virtual mem (1G below kernel)
    uint64_t virtaddr = 0xffffffff40200000;
    for (uint64_t physaddr = tag_fb->framebuffer.addr;
         physaddr < tag_fb->framebuffer.addr + fb_size;
         physaddr += HUGEPAGE_SIZE, virtaddr += HUGEPAGE_SIZE) {
        uint64_t *pdpt;
        uint64_t *pd;
        uint64_t *pt;
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
            pdpt[PDPT_INDEX(virtaddr)] =
                (uint64_t)pd | PTE_PRESENT | PTE_READWRITE;
        }
        pd = PML4E_TO_ADDR(pdpt[PDPT_INDEX(virtaddr)]);
        pd[PD_INDEX(virtaddr)] =
            physaddr | PDE_HUGE | PTE_PRESENT | PTE_READWRITE;
    }
    printf("Done.\n");

    /// Reload CR3 with our new PML4 mapping
    __asm__ volatile(
        "movq %0, %%cr3" ::"r"((uint64_t)pml4 - 0xffffffff80000000));
}

int pmem_cmp(const struct pmem_block *a, const struct pmem_block *b)
{
    // We're dealing with potentially huge numbers
    if (a->base > b->base) {
        return 1;
    } else if (a->base < b->base) {
        return -1;
    } else {
        return 0;
    }
}

/**
 * Determine what physical memory is available given the memory map from the bootloader,
 * then setup our physical memory manager so we can start allocating.
 * mb2_info will NOT be available after this subroutine returns.
 */
void pmem_init(struct mb2_info *mb2_info)
{
    struct mb2_tag *tag = mb2_find_tag(mb2_info, MB_TAG_TYPE_MEMORY_MAP);
    if (tag == NULL) {
        // TODO: Panic
        printf("Failed to find memory map in boot info.\n");
        return;
    }

    // Gather available RAM from MB2 memory map
    for (struct mb2_memory_map_entry *mm_entry = tag->memory_map.entries;
         (uint8_t *)mm_entry < (uint8_t *)tag + tag->size;
         mm_entry =
             (struct mb2_memory_map_entry *)((unsigned long)mm_entry +
                                             tag->memory_map.entry_size)) {
        if (pmem_count >= MAX_PMEM_ENTRIES) {
            // We're out of early-allocated space!
            break;
        }

        // MB2 Spec Section 3.6.8
        // type 1 indicates available RAM, anything else is reserved or unusable
        if (mm_entry->type != 1) {
            continue;
        }

        // Base address, aligned up to nearest page boundary
        uint64_t base = mm_entry->base_addr;
        base += (base % PAGE_SIZE);
        if (base == 0) {
            // Don't map the lowest page
            // So we can use it as a condition for "unmapped"
            base += PAGE_SIZE;
        }
        // Limit address, aligned down to nearest page boundary
        uint64_t limit = mm_entry->base_addr + mm_entry->length;
        limit -= (limit % PAGE_SIZE);
        // Total block size, multiple of page size
        uint64_t size = limit - base;
        if (size == 0 || base >= limit) {
            continue;
        }

        // Record available block of RAM
        struct pmem_block *block = pmem_map + pmem_count;
        pmem_count += 1;
        block->base = base;
        block->limit = limit;
    }

    // Sort mapped blocks by base
    qsort(pmem_map, pmem_count, sizeof(*pmem_map), pmem_cmp);

    // Merge overlapping blocks
    bool overlap;
    do {
        overlap = false;
        for (size_t i = 0; i < pmem_count; i++) {
            for (size_t j = i; j < pmem_count; j++) {
                if (i == j) {
                    continue;
                }

                struct pmem_block *p_i = pmem_map + i;
                struct pmem_block *p_j = pmem_map + j;
                // Memory overlap: 2 cases
                if ((p_i->base <= p_j->base && p_i->limit > p_j->base) ||
                    (p_j->base <= p_i->base && p_j->limit > p_i->base)) {
                    overlap = true;
                }
                if (overlap) {
                    // Take min(base_i, base_j) as new base
                    // Take max(limit_i, limit_j) as new limit
                    // Assign new base and limit to block i
                    uint64_t merged_base =
                        p_i->base < p_j->base ? p_i->base : p_j->base;
                    uint64_t merged_limit =
                        p_i->limit > p_j->limit ? p_i->limit : p_j->limit;
                    p_i->base = merged_base;
                    p_i->limit = merged_limit;
                    // block j is now unused, shift everything (to the right of j) left
                    for (size_t k = j; k < pmem_count - 1; k++) {
                        *(pmem_map + k) = *(pmem_map + k + 1);
                    }
                    pmem_count -= 1;
                    goto break_overlap_loop; // break nested loop
                }
            }
        }
    break_overlap_loop:;
    } while (overlap);

    // Mark physical alloc map as not free ("allocated") by default
    for (size_t i = 0; i < PMEM_MAP_MAX_POOL; i++)
        *(pmem_alloc_map + i) =
            PMEM_ALLOC_MAP_UNINITIALISED | PMEM_ALLOC_MAP_USED;

    // Summary & mark free pages
    size_t total_block_size = 0;
    for (size_t i = 0; i < pmem_count; i++) {
        struct pmem_block *block = pmem_map + i;
        size_t block_size = block->limit - block->base;
        total_block_size += block_size;
        printf("Available RAM %x .. %x (%u MiB)\n", block->base, block->limit,
               block_size / (1 << 20));
        pmem_free(block->base, block->limit);
    }
    printf("Total physical memory entries mapped: %u (%u GiB)\n\n", pmem_count,
           total_block_size / (1 << 30));

    // We now have some memory to allocate for our page tables
    paging_init(mb2_info);
}
