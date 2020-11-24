#ifndef __ARGIR__PAGING_H
#define __ARGIR__PAGING_H

#include "addr.h"
#include "mb2.h"
#include "pmem.h"

#define PAGE_SIZE (0x1000) /** 4K */
#define HUGEPAGE_SIZE (0x200000) /** 2M */

// PML4 is allocated in `paging_init`
uint64_t pml4[512] __attribute__((aligned(4096)));
// Limit of the linear address space after `paging_init`
uint64_t linear_limit;

void paging_init(struct mb2_info *mb2_info);

#endif /* __ARGIR__PAGING_H */
