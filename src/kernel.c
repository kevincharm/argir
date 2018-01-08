#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include "terminal.h"

void kernel_main()
{
    terminal_init();
    printf(
        "Argir i386\n"
        "Build "__ARGIR_BUILD_COMMIT__"\n\n"
    );
    for (uint8_t i=0; i<80; i++) {
        putchar('0'+i);
    }
}
