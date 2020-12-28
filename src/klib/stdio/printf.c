#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

void terminal_set_bg_colour(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void terminal_set_fg_colour(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/**
 * Convert a number to a regular ol' ASCII string.
 */
char *ulltoa(unsigned long long num, char *str, int base)
{
    char *a = str;

    // Base case
    if (num == 0) {
        *a = '0';
        a += 1;
        *a = 0;
        a += 1;
        return str;
    }

    // Keep dividing by base to get each digit (backwards, LSB->MSB)
    for (; num > 0; a += 1, num /= base) {
        int r = num % base;
        if (r >= 10) {
            // Cannot be represented as single char using base 10 numerals
            *a = 'a' + (r - 10);
        } else {
            *a = '0' + r;
        }
    }

    // Reverse it
    char *begin = str;
    char *end = a - 1;
    for (; begin < end; begin += 1, end -= 1) {
        char tmp = *begin;
        *begin = *end;
        *end = tmp;
    }

    // Null terminator.
    *a = 0;

    return str;
}

int printf(const char *restrict fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    int len = 0;
    while (*fmt != '\0') {
        if (*fmt == '\x1b' && *(fmt + 1) == '[') {
            // ANSI escape
            fmt += 2;
            // SGR functions
            if (*fmt == '0' && *(fmt + 1) == 'm') {
                // Reset
                terminal_set_bg_colour(0, 0, 0, 0xff);
                terminal_set_fg_colour(0xff, 0xff, 0xff, 0xff);
                fmt += 2;
                continue;
            } else if (*fmt == '3' && *(fmt + 2) == 'm') {
                // Set text to basic colour palette
                uint8_t colour = *(fmt + 1) - '0';
                switch (colour) {
                case 0: // Black
                    terminal_set_fg_colour(0, 0, 0, 0xff);
                    break;
                case 1: // Red
                    terminal_set_fg_colour(0xff, 0, 0, 0xff);
                    break;
                case 2: // Green
                    terminal_set_fg_colour(0, 0xff, 0, 0xff);
                    break;
                case 3: // Yellow
                    terminal_set_fg_colour(0xff, 0xff, 0, 0xff);
                    break;
                case 4: // Blue
                    terminal_set_fg_colour(0, 0, 0xff, 0xff);
                    break;
                case 5: // Magenta
                    terminal_set_fg_colour(0xff, 0, 0xff, 0xff);
                    break;
                case 6: // Cyan
                    terminal_set_fg_colour(0, 0xff, 0xff, 0xff);
                    break;
                case 7: // White
                    terminal_set_fg_colour(0xff, 0xff, 0xff, 0xff);
                    break;
                }
                fmt += 3;
                continue;
            } else if (*fmt == '4' && *(fmt + 2) == 'm') {
                // Set background to basic colour palette
                uint8_t colour = *(fmt + 1) - '0';
                switch (colour) {
                case 0: // Black
                    terminal_set_bg_colour(0, 0, 0, 0xff);
                    break;
                case 1: // Red
                    terminal_set_bg_colour(0xff, 0, 0, 0xff);
                    break;
                case 2: // Green
                    terminal_set_bg_colour(0, 0xff, 0, 0xff);
                    break;
                case 3: // Yellow
                    terminal_set_bg_colour(0xff, 0xff, 0, 0xff);
                    break;
                case 4: // Blue
                    terminal_set_bg_colour(0, 0, 0xff, 0xff);
                    break;
                case 5: // Magenta
                    terminal_set_bg_colour(0xff, 0, 0xff, 0xff);
                    break;
                case 6: // Cyan
                    terminal_set_bg_colour(0, 0xff, 0xff, 0xff);
                    break;
                case 7: // White
                    terminal_set_bg_colour(0xff, 0xff, 0xff, 0xff);
                    break;
                }
                fmt += 3;
                continue;
            }
        }

        if (*fmt == '%' && *(fmt + 1) == '%') {
            putchar(*((const unsigned char *)fmt));
            fmt += 2;
            len += 1;
            continue;
        }

        if (*fmt != '%') {
            putchar(*((const unsigned char *)fmt));
            fmt += 1;
            len += 1;
            continue;
        }

        // %*
        fmt += 1;

        if (*fmt == 'c') {
            int c = va_arg(ap, int);
            putchar(*((const unsigned char *)c));
            fmt += 1;
            len += 1;
            continue;
        }

        if (*fmt == 's') {
            const char *str = va_arg(ap, const char *);
            for (size_t i = 0; i < strlen(str); i++) {
                putchar(*((const unsigned char *)(str + i)));
                len += 1;
            }
            fmt += 1;
            continue;
        }

        // Unsigned (base 10)
        if (*fmt == 'u') {
            unsigned long long u = va_arg(ap, unsigned long long);
            char ubuf[21];
            char *ubuf_out = ulltoa(u, ubuf, 10);
            for (; *ubuf_out; ubuf_out += 1, len += 1) {
                putchar(*ubuf_out);
            }
            fmt += 1;
            continue;
        }

        // Hex
        if (*fmt == 'x') {
            unsigned long long x = va_arg(ap, unsigned long long);
            char xbuf[17];
            char *xbuf_out = ulltoa(x, xbuf, 16);
            for (; *xbuf_out; xbuf_out += 1, len += 1) {
                putchar(*xbuf_out);
            }
            fmt += 1;
            continue;
        }
    }

    va_end(ap);
    return len;
}
