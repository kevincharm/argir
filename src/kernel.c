#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <kernel/cpu.h>
#include <kernel/interrupts.h>
#include <kernel/terminal.h>
#include <kernel/keyboard.h>

void kernel_main()
{
    terminal_init();
    printf(
        "Argir i386\n"
        "Build "__ARGIR_BUILD_COMMIT__"\n\n"
    );

    cli(); // Disable interrupts

    gdt_init();
    pic_remap();
    idt_load();
    irq_init();
    keyboard_init();
    pic_irq_on(1);

    sti(); // Enable interrupts
    printf("Interrupts are %s.\n",
        irqs_enabled() ? "ENABLED" : "DISABLED");

    for (;;) {
        // keyboard_main();
    }
}
