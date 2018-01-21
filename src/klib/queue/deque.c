#include <queue.h>
#include <memory.h>

void u8_lifo_push(struct u8_deque *d, uint8_t data)
{
    *(d->buffer + d->head) = data;
    d->head = (d->head + 1) % DEQUE_BUFSIZ;
    d->len += 1;
}

uint8_t u8_lifo_pop(struct u8_deque *d)
{
    // Needs to be atomic wrt IRQs
    uint8_t ret = *(d->buffer + d->tail);
    d->tail = (d->tail + 1) % DEQUE_BUFSIZ;
    d->len -= 1;
    return ret;
}

void u8_lifo_init(struct u8_deque *d)
{
    memset(d->buffer, 0, DEQUE_BUFSIZ);
    d->head = 0;
    d->tail = 0;
    d->len = 0;
}
