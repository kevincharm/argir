#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include "kernel/mb2.h"
#include "kernel/cpu.h"
#include "kernel/interrupts.h"
#include "kernel/terminal.h"
#include "kernel/keyboard.h"
#include "kernel/pci.h"

#ifndef __ARGIR_BUILD_COMMIT__
#define __ARGIR_BUILD_COMMIT__ "balls"
#endif

struct pci g_pci;
uint32_t mb2_magic;
uint32_t mb2_info;

static void init_pci()
{
    struct pci *pci = &g_pci;
    pci_init(pci);
    for (int i = 0; i < pci->dev_count; i++) {
        struct pci_descriptor *p = pci->dev + i;
        printf("PCI device <vendor: %u, device: %u>\n", p->vendor_id,
               p->device_id);
    }
}

static void print_logo()
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

void kernel_main()
{
    interrupts_disable();

    if (mb2_magic != 0x36d76289) {
        /** wat do?! */
        __asm__ volatile("mov $0xdeadbeef, %rax\n\t"
                         "hlt");
    }

    struct mb2_tag *mb2_tag_fb = mb2_find_tag(mb2_info, 8);
    uint8_t *fb = ((void *)(mb2_tag_fb->framebuffer.addr));

    terminal_init(fb, mb2_tag_fb->framebuffer.width,
                  mb2_tag_fb->framebuffer.height,
                  mb2_tag_fb->framebuffer.pitch);
    print_logo();

    gdt_init();
    interrupts_init();
    keyboard_init();
    init_pci();

    interrupts_enable();

    for (;;) {
        keyboard_main();

        __asm__ volatile("hlt");
    }
}
