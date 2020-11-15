#!/usr/bin/env python3
import sys


def main():
    linear_addr = int(sys.argv[1], 16)

    PAGE_OFFSET_BITS = 12
    PTE_BITS = 9
    PDE_BITS = 9
    PDPTE_BITS = 9
    PML4E_BITS = 9

    pml4_index = (linear_addr >> (
        PAGE_OFFSET_BITS + PTE_BITS + PDE_BITS + PDPTE_BITS)) & 0x1ff
    pdpt_index = (linear_addr >> (
        PAGE_OFFSET_BITS + PTE_BITS + PDE_BITS)) & 0x1ff
    pd_index = (linear_addr >> (PAGE_OFFSET_BITS + PTE_BITS)) & 0x1ff
    page_off = (linear_addr & 0xfff)

    print(f"PML4 index:\t{pml4_index}")
    print(f"PDPT index:\t{pdpt_index}")
    print(f"PD index:\t{pd_index}")
    print(f"Page offset:\t{page_off}")


try:
    main()
except:
    print("Linear address to page table indices calculator\n")
    print("\tUsage:\n\t\tlinear.py <hex address>\n")
    print("\tExample:\n\t\tlinear.py 0xffffffff80200000\n")
