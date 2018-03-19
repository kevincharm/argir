#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static inline int pow10(unsigned int x) {
    unsigned int base = 10;
    int ret = 1;
    while (x) {
        if (x & 0x01)
            ret *= base;
        base *= base;
        x >>= 1;
    }
    return ret;
}

static inline int uintlen(unsigned int u) {
    int len = 0;
    unsigned int num = u;
    while (num) {
        num /= 10;
        len += 1;
    }
    return len;
}

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

        if (*fmt == 'u') {
            unsigned int u = va_arg(ap, unsigned int);
            int ulen = uintlen(u);
            if (!ulen) {
                putchar('0');
            }
            for (size_t i=0; i<(size_t)ulen; i++) {
                int tens = pow10(ulen-1-i);
                int n = u/tens;
                putchar('0'+n);
                u -= n * tens;
                len += 1;
            }
            fmt += 1;
            continue;
        }
    }

    va_end(ap);
    return len;
}
