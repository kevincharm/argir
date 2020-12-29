#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include "kernel/addr.h"
#include "kernel/mb2.h"
#include "kernel/cpu.h"
#include "kernel/interrupts.h"
#include "kernel/terminal.h"
#include "kernel/keyboard.h"
#include "kernel/pci.h"
#include "kernel/pmem.h"
#include "kernel/paging.h"

#ifndef __ARGIR_BUILD_COMMIT__
#define __ARGIR_BUILD_COMMIT__ "balls"
#endif

uint32_t mb2_magic;
uint32_t mb2_info;

static void print_build_info();

void kernel_main(void)
{
    interrupts_disable();

    // MB2 boot info check
    if (mb2_magic != 0x36d76289) {
        __asm__ volatile("mov $0xbaaaaaadb0000007, %rax\n\t"
                         "hlt"); // MB2 did not drop us here!
    }
    // Calculate higher-half MB2 boot info address
    uint64_t mb2_info_vma = (uint64_t)mb2_info + KERNEL_VMA;

    terminal_init(NULL, 800, 600, 3200, 1); // Dummy null output for printfs
    pmem_init(mb2_info_vma);
    paging_init(mb2_info_vma);
    vmem_init();
    print_build_info();

    gdt_init();
    interrupts_init();
    keyboard_init();

    // Ready to go
    interrupts_enable();

    for (;;) {
        keyboard_main();

        __asm__ volatile("hlt");
    }
}

static void print_build_info()
{
    printf(
        "\n                                @@\\\n"
        "                                \\__|\n"
        "   @@@@@@\\   @@@@@@\\   @@@@@@\\  @@\\  @@@@@@\\   @@@@@@\\   @@@@@@@\\\n"
        "   \\____@@\\ @@  __@@\\ @@  __@@\\ @@ |@@  __@@\\ @@  __@@\\ @@  _____|\n"
        "   @@@@@@@ |@@ |  \\__|@@ /  @@ |@@ |@@ |  \\__|@@ /  @@ |\\@@@@@@\\\n"
        "  @@  __@@ |@@ |      @@ |  @@ |@@ |@@ |      @@ |  @@ | \\____@@\\\n"
        "  \\@@@@@@@ |@@ |      \\@@@@@@@ |@@ |@@ |      \\@@@@@@  |@@@@@@@  |\n"
        "   \\_______|\\__|       \\____@@ |\\__|\\__|       \\______/ \\_______/\n"
        "                      @@\\   @@ |\n"
        "                      \\@@@@@@  |\n"
        "                       \\______/\n\n"
        "Argir x86_64\n"
        "Build " __ARGIR_BUILD_COMMIT__ "\n\n");
}
