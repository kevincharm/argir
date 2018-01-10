#include <stdio.h>
#include <kernel/io.h>
#include <kernel/keyboard.h>

static char kb_scan()
{
    char ret;
    do {
        ret = inb(IRQ_PORT_STATUS);
    } while (!(ret & 1));
    ret = (char)inb(IRQ_SCAN_CODE);
    return ret;
}

void keyboard_main()
{
    putchar(kb_scan());
}
