#ifndef __ARGIR__PMEM_H
#define __ARGIR__PMEM_H

#include <stddef.h>
#include "mb2.h"
#include "paging.h"

struct pmem_block {
    uint64_t base;
    uint64_t limit;
};

#define MAX_PMEM_ENTRIES (128) /** 128 * 16B = 2K, enough for now? */
/// Memory map from MB2 boot info, sorted and merged
struct pmem_block pmem_block_map[MAX_PMEM_ENTRIES];
size_t pmem_blocks_count;

void *pmem_alloc_page();
void pmem_free_range(uint64_t base, uint64_t limit);
void pmem_init(struct mb2_info *mb2_info);

#endif /* __ARGIR__PMEM_H */
