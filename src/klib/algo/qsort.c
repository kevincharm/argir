#include <algo.h>
#include <memory.h>

#define VPTR(arr, index, bytes_per_entry)                                      \
    ((uint8_t *)arr + (index * bytes_per_entry))

static void _qsort_swap(void *a, void *b, const size_t bytes_per_entry)
{
    uint8_t temp[bytes_per_entry];
    memcpy(temp, a, bytes_per_entry);
    memcpy(a, b, bytes_per_entry);
    memcpy(b, temp, bytes_per_entry);
}

static void _qsort(void *arr, size_t n, const size_t bytes_per_entry,
                   int (*cmp_func)(const void *a, const void *b), size_t left,
                   size_t right)
{
    if (left >= right) {
        return;
    }

    size_t mid = (left + right) / 2;
    void *low = VPTR(arr, left, bytes_per_entry);
    void *high = VPTR(arr, mid, bytes_per_entry);
    _qsort_swap(low, high, bytes_per_entry);

    void *pivot;
    size_t p = left;
    for (size_t i = left + 1; i <= right; i++) {
        void *curr = VPTR(arr, i, bytes_per_entry);
        if (cmp_func(low, curr) > 0) {
            p += 1;
            pivot = VPTR(arr, p, bytes_per_entry);
            _qsort_swap(curr, pivot, bytes_per_entry);
        }
    }
    pivot = VPTR(arr, p, bytes_per_entry);
    _qsort_swap(low, pivot, bytes_per_entry);

    // Note: `p` is unsigned, so don't let it underflow.
    _qsort(arr, n, bytes_per_entry, cmp_func, left, p > 0 ? p - 1 : left);
    _qsort(arr, n, bytes_per_entry, cmp_func, p + 1, right);
}

void qsort(void *arr, size_t n, size_t bytes_per_entry,
           int (*cmp_func)(const void *a, const void *b))
{
    _qsort(arr, n, bytes_per_entry, cmp_func, 0, n - 1);
}
