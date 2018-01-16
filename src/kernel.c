#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <kernel/irq.h>
#include <kernel/terminal.h>
#include <kernel/keyboard.h>

void kernel_main()
{
    irq_init();
    keyboard_init();
    terminal_init();
    printf(
        "Argir i386\n"
        "Build "__ARGIR_BUILD_COMMIT__"\n\n"
    );

    printf("Interrupts are %s.\n",
        irqs_enabled() ? "ENABLED" : "DISABLED");

    for (;;) {
        keyboard_main();
    }
}
