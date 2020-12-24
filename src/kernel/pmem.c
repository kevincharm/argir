#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <memory.h>
#include <algo.h>
#include "kernel/pmem.h"
#include "kernel/paging.h"

// Number of contiguous blocks of RAM
size_t pmem_blocks_count = 0;
// Number of free 4K pages
size_t usable_pages = 0;
// Index of the free block currently giving allocations
size_t pmem_current_block = 0;

/**
 * Return an available physical page (initialised with zeros).
 */
void *pmem_alloc_page()
{
    struct pmem_block *block = pmem_block_map + pmem_current_block;

    // First check if we have enough space to allocate from this block
    if (block->limit - block->base < PAGE_SIZE) {
        // Not enough space left in this block, move on to the previous
        pmem_current_block -= 1;
        if (pmem_current_block < 0) {
            /// TODO: PANIC
            printf("Out of physical memory!\n");
            __asm__ volatile("hlt");
            return;
        }

        // Try again with new block index
        return pmem_alloc_page();
    }

    block->limit -= PAGE_SIZE;
    return block->limit;
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
        if (pmem_blocks_count >= MAX_PMEM_ENTRIES) {
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
        if (base % PAGE_SIZE != 0) {
            base += PAGE_SIZE - (base % PAGE_SIZE);
        }
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
        struct pmem_block *block = pmem_block_map + pmem_blocks_count;
        pmem_blocks_count += 1;
        block->base = base;
        block->limit = limit;
    }

    // Sort mapped blocks by base
    qsort(pmem_block_map, pmem_blocks_count, sizeof(*pmem_block_map), pmem_cmp);

    // Merge overlapping blocks
    bool overlap;
    do {
        overlap = false;
        for (size_t i = 0; i < pmem_blocks_count; i++) {
            for (size_t j = i + 1; j < pmem_blocks_count; j++) {
                struct pmem_block *p_i = pmem_block_map + i;
                struct pmem_block *p_j = pmem_block_map + j;
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
                    for (size_t k = j; k < pmem_blocks_count - 1; k++) {
                        *(pmem_block_map + k) = *(pmem_block_map + k + 1);
                    }
                    pmem_blocks_count -= 1;
                    goto break_overlap_loop; // break nested loop
                }
            }
        }
    break_overlap_loop:;
    } while (overlap);

    // Summary & mark free pages
    size_t total_block_size = 0;
    for (size_t i = 0; i < pmem_blocks_count; i++) {
        struct pmem_block *block = pmem_block_map + i;
        size_t block_size = block->limit - block->base;
        total_block_size += block_size;
        printf("Available RAM %x .. %x (%u MiB)\n", block->base, block->limit,
               block_size / (1 << 20));
        // We will allocate physical pages backwards starting from the last block
        pmem_current_block = i;
    }
    printf("Total physical memory entries mapped: %u (%u GiB)\n\n",
           pmem_blocks_count, total_block_size / (1 << 30));

    // We now have some memory to allocate for our page tables
    paging_init(mb2_info);
}
