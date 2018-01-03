#ifndef _STRING_H
#define _STRING_H

/**
 *  Ref:
 *  http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/string.h.html
 */

#include <stddef.h>

int memcmp(const void *, const void *, size_t);
void *memcpy(void *restrict, const void *restrict, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
size_t strlen(const char *);

#endif /* _STRING_H */
