#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <memory.h>
#include <algo.h>
#include <kernel/mb2.h>

// Page table utils
#define PAGE_OFFSET_BITS (12)
#define PTE_BITS (9)
#define PDE_BITS (9)
#define PDPTE_BITS (9)
#define PML4E_BITS (9)
#define PML4_INDEX(linear_addr)                                                \
    ((linear_addr >> (PAGE_OFFSET_BITS + PTE_BITS + PDE_BITS + PDPTE_BITS)) &  \
     0x1ff)
#define PDPT_INDEX(linear_addr)                                                \
    ((linear_addr >> (PAGE_OFFSET_BITS + PTE_BITS + PDE_BITS)) & 0x1ff)
#define PD_INDEX(linear_addr)                                                  \
    ((linear_addr >> (PAGE_OFFSET_BITS + PTE_BITS)) & 0x1ff)
#define PT_INDEX(linear_addr) ((linear_addr >> PAGE_OFFSET_BITS) & 0x1ff)
#define PAGE_OFF(linear_addr) ((linear_addr & 0xfff))

#define PAGE_SIZE (0x1000) /** 4K */

struct pmem_block {
    uint64_t base;
    uint64_t limit;
};

// 1GiB / 4K = 262,144 pages
// 262,144 pages / 8 bits = 32K bitmap space per GiB
#define PMEM_BITMAP_MAX_POOL (16 * 32768) /** 512K pool -> 16G addressable */
/// Early static physical memory bitmap
// Set bit -> allocated
// Clear bit -> free
uint8_t pmem_alloc_bitmap[PMEM_BITMAP_MAX_POOL];

#define MAX_PMEM_ENTRIES (128) /** 128 * 16B = 2K, enough for now? */
/// Memory map from MB2 boot info, sorted and merged
struct pmem_block pmem_map[MAX_PMEM_ENTRIES];
size_t pmem_count = 0;

// TODO: Allocate this?
uint64_t pml4[512];

void paging_init(struct mb2_info *mb2_info)
{
    /// Map the linear framebuffer ///
    struct mb2_tag *tag_fb = mb2_find_tag(mb2_info, MB_TAG_TYPE_FRAMEBUFFER);
    if (tag_fb == NULL) {
        // TODO: Panic
        printf("Failed to find framebuffer tag in boot info.\n");
        return;
    }

    // Bytes, pages needed for entire framebuffer
    const uint64_t fb_page_size = 0x200000; // Use 2M hugepages for LFB
    size_t fb_size = tag_fb->framebuffer.pitch * tag_fb->framebuffer.height;
    size_t fb_pages = fb_size / fb_page_size;
    if (fb_size > 0x40000000) {
        // We assume that the entire LFB fits inside 1GB
        // TODO: Panic
        printf("LFB is larger than 1GiB!\n");
        return;
    }

    // TODO: Allocate a PDPT for this PML4 entry
    uint64_t *pdpt;
    // TODO: Allocate a PD for this PDPT
    uint64_t *pd;

    // Convert the LFB identity-mapped linear address to L4 page indices
    uint64_t physaddr = tag_fb->framebuffer.addr;
    // We'll map the LFB to -3G in virtual mem (1G below kernel)
    uint64_t virtaddr = 0xffffffff40200000;
    // Initial mappings: we know that resolutions upto 4K will fit inside 1 PD with 2M pages
    pdpt[PDPT_INDEX(virtaddr)] = pd[PD_INDEX(virtaddr)];
    pml4[PML4_INDEX(virtaddr)] = pdpt[PDPT_INDEX(virtaddr)];
    for (; physaddr < fb_size;
         physaddr += fb_page_size, virtaddr += fb_page_size) {
        // Mark as HUGE | PRESENT | WRITABLE
        pd[PD_INDEX(virtaddr)] = physaddr | 0b10000011;
    }

    /// Drop the lower 4G identity mapping ///
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

void *pmem_alloc(size_t bytes)
{
    if (bytes == 0) {
        // wat
        // TODO: Panic
        return 0xbaadbeef;
    }

    // Pages needed == clear bits needed
    size_t pages = bytes / PAGE_SIZE;
    if (bytes % PAGE_SIZE != 0) {
        pages += 1;
    }

    // Find a free slice, mark it as allocated
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
        // Each bitmap (=8b) holds 8 pages
        byte_off = page_off / (PAGE_SIZE << 3);
        if (byte_off >= PMEM_BITMAP_MAX_POOL) {
            printf("Ran out of bitmap memory pool! (byte_off = %u)\n",
                   byte_off);
            return;
        }
        *(pmem_alloc_bitmap + byte_off) = 0;
    }
    // Leftover bits
    byte_off = page_off / (PAGE_SIZE << 3);
    if (byte_off >= PMEM_BITMAP_MAX_POOL) {
        return;
    }
    uint8_t *bitmap = pmem_alloc_bitmap + byte_off;
    *bitmap = 0;
    size_t bit_off = page_off % (PAGE_SIZE << 3);
    while (bit_off--)
        *bitmap &= ~(1 << bit_off);
    goto done;

oom:
    printf("Ran out of bitmap memory pool! (byte_off = %u)\n", byte_off);
done:
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

    // Mark bitmap as not free ("allocated") by default
    for (size_t i = 0; i < PMEM_BITMAP_MAX_POOL; i++)
        *(pmem_alloc_bitmap + i) = 0xff;

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
}
