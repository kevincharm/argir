#include <stdarg.h>
#include <stdio.h>
#include <string.h>

int printf(const char *restrict fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    int len = 0;
    while (*fmt != '\0') {
        if (*fmt == '%' && *(fmt+1) == '%') {
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
            for (size_t i=0; i<strlen(str); i++) {
                putchar(*((const unsigned char *)(str+i)));
                len += 1;
            }
            fmt += 1;
            continue;
        }
    }

    va_end(ap);
    return len;
}
