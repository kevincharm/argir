#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <kernel/cpu.h>
#include <kernel/interrupts.h>
#include <kernel/terminal.h>
#include <kernel/keyboard.h>
#include <kernel/pci.h>

struct pci g_pci;

void init_pci()
{
    struct pci *pci = &g_pci;
    pci_init(pci);
    for (int i=0; i<pci->dev_count; i++) {
        struct pci_descriptor *p = pci->dev+i;
        printf("PCI device <vendor: %u, device: %u>\n",
            p->vendor_id, p->device_id);
    }
}

void kernel_main()
{
    terminal_init();
    printf(
        "Argir i386\n"
        "Build "__ARGIR_BUILD_COMMIT__"\n\n"
    );

    interrupts_disable();

    gdt_init();
    interrupts_init();
    keyboard_init();
    init_pci();

    interrupts_enable();

    for (;;) {
        keyboard_main();
    }
}
