#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "terminal.h"

void kernel_main()
{
    struct terminal term;
    struct terminal *p_term = &term;
    terminal_init(p_term);
    terminal_write(p_term,
        "Argir i386\n"
        "Build "__ARGIR_BUILD_COMMIT__"\n\n"
    );
    for (uint8_t i=0; i<80; i++) {
        terminal_write_char(p_term, '0'+i);
    }
}
