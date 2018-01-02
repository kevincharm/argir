#ifndef _STDIO_H
#define _STDIO_H

/**
 *  Ref:
 *  http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/stdio.h.html
 */

#define EOF         (-1)
#define BUFSIZ      (512)

int printf(const char *restrict, ...);
int putchar(int);
int puts(const char *);

#endif /* _STDIO_H */
