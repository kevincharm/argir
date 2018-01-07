#include <stdio.h>

extern void terminal_write_char(const char str);

int putchar(int c)
{
    terminal_write_char((const char)c);
    return c;
}
