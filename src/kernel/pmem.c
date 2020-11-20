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

// We need memory to map memory!
#define MAX_PMEM_ENTRIES (128) /** 128 * 16B = 2K, enough for now? */
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
        // printf("Available physical @ base: 0x%x, limit: 0x%x, size: %u\n",
        //        block->base, block->limit, size);
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
                // Memory overlap
                if (p_i->base <= p_j->base && p_i->limit > p_j->base) {
                    /*
                     *  xxxx        xx
                     *    xxxx      xxxx
                     */
                    overlap = true;
                } else if (p_j->base <= p_i->base && p_j->limit > p_i->base) {
                    /*
                     *    xxxx      xxxx
                     *  xxxx        xx
                     */
                    overlap = true;
                }
                if (overlap) {
                    // printf("Overlap regions: [%x, %x] and [%x, %x]\n",
                    //        p_i->base, p_i->limit, p_j->base, p_j->limit);
                    uint64_t merged_base =
                        p_i->base < p_j->base ? p_i->base : p_j->base;
                    uint64_t merged_limit =
                        p_i->limit > p_j->limit ? p_i->limit : p_j->limit;
                    p_i->base = merged_base;
                    p_i->limit = merged_limit;
                    // p_j is now unused, shift everything left
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

    size_t total_block_size = 0;
    for (size_t i = 0; i < pmem_count; i++) {
        struct pmem_block *block = pmem_map + i;
        size_t block_size = block->limit - block->base;
        total_block_size += block_size;
        printf("Available RAM (%u B) .... %x .... %x\n", block_size,
               block->base, block->limit);
    }
    printf("Total physical memory entries mapped: %u (%u B)\n\n", pmem_count,
           total_block_size);
}
