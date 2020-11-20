#ifndef _ALGO_H
#define _ALGO_H

#include <stdint.h>
#include <stddef.h>

void qsort(void *arr, size_t n, size_t bytes_per_entry,
           int (*cmp_func)(const void *a, const void *b));

#endif