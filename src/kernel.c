#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <kernel/cpu.h>
#include <kernel/interrupts.h>
#include <kernel/terminal.h>
#include <kernel/keyboard.h>
#include <kernel/pci.h>

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

    interrupts_enable();

    pci_init();

    for (;;) {
        keyboard_main();
    }
}
