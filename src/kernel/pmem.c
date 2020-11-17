#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
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
#define PAGE_OFF(linear_addr) ((linear_addr & 0xfff))

#define PAGE_SIZE (0x1000) /** 4K */

struct pmem_block {
    uint64_t base;
    uint64_t limit;
};

// We need memory to map memory!
#define MAX_PMEM_ENTRIES (128) /** 128 * 16B = 2K, enough for now? */
struct pmem_block pmem_map[MAX_PMEM_ENTRIES];
size_t pmem_count = 0;

/**
 * Determine what physical memory is available given the memory map from the bootloader,
 * then setup our physical memory manager so we can start allocating.
 * mb2_info will NOT be available after this subroutine returns.
 */
void pmem_init(struct mb2_info *mb2_info)
{
    struct mb2_tag *tag = mb2_find_tag(mb2_info, MB_TAG_TYPE_MEMORY_MAP);
    if (tag == NULL) {
        // TODO: panic
        printf("Failed to find memory map in boot info.\n");
        return;
    }

    size_t total_bytes_mapped = 0;
    for (struct mb2_memory_map_entry *mm_entry = tag->memory_map.entries;
         (uint8_t *)mm_entry < (uint8_t *)tag + tag->size;
         mm_entry =
             (struct mb2_memory_map_entry *)((uint32_t)mm_entry +
                                             tag->memory_map.entry_size)) {
        if (pmem_count >= MAX_PMEM_ENTRIES) {
            // We're out of early-allocated space!
            break;
        }

        // MB2 Spec Section 3.6.8
        // type value of 1 indicates available RAM
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
        if (size == 0) {
            continue;
        }

        // Record available block of RAM
        struct pmem_block *block = pmem_map + pmem_count;
        pmem_count += 1;
        block->base = base;
        block->limit = limit;
        total_bytes_mapped += size;
        // printf("Available physical @ base: 0x%x, limit: 0x%x, size: %u\n",
        //        block->base, block->limit, size);
    }
    printf("Total physical memory entries mapped: %u (%u bytes)\n", pmem_count,
           total_bytes_mapped);
}
