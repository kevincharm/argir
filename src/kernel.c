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

    interrupts_disable();

    gdt_init();
    pic_remap();
    idt_load();
    interrupts_init();
    keyboard_init();

    interrupts_enable();

    for (;;) {
        keyboard_main();
    }
}
