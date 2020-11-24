#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <memory.h>
#include <algo.h>
#include "kernel/pmem.h"

// 1GiB / 4K = 262,144 pages
// 262,144 pages = 256K bitmap space per GiB
#define PMEM_MAP_MAX_POOL (16 * 262144) /** 4M pool -> 16G addressable */
/// Early static physical alloc map
uint8_t pmem_alloc_map[PMEM_MAP_MAX_POOL];
#define PMEM_ALLOC_MAP_USED (1 << 0)
#define PMEM_ALLOC_MAP_UNINITIALISED (1 << 7)

size_t pmem_count = 0;

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
void pmem_free_range /** chicken */ (uint64_t base, uint64_t limit)
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

static int pmem_cmp(const struct pmem_block *a, const struct pmem_block *b)
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
        pmem_free_range(block->base, block->limit);
    }
    printf("Total physical memory entries mapped: %u (%u GiB)\n\n", pmem_count,
           total_block_size / (1 << 30));

    // We now have some memory to allocate for our page tables
    paging_init(mb2_info);
}
