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

#ifndef __ARGIR_BUILD_COMMIT__
#define __ARGIR_BUILD_COMMIT__ "balls"
#endif

uint32_t mb2_magic;
uint32_t mb2_info;

static void print_build_info();

void kernel_main(void)
{
    interrupts_disable();
    if (mb2_magic != 0x36d76289) {
        __asm__ volatile("mov $0xbaaaaaadb0000007, %rax\n\t"
                         "hlt"); // MB2 did not drop us here!
    }

    // Extract required info from MB2 bootinfo before it disappears
    struct mb2_tag *mb2_tag_fb = mb2_find_tag(mb2_info, 8);
    size_t width = mb2_tag_fb->framebuffer.width;
    size_t height = mb2_tag_fb->framebuffer.height;
    size_t pitch = mb2_tag_fb->framebuffer.pitch;

    pmem_init(mb2_info);
    terminal_init(LFB_VMA, width, height, pitch);
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
