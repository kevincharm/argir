#include <ringbuf.h>
#include <memory.h>

bool u8_rb_fifo_has_data(struct u8_ringbuf *d)
{
    return d->p_write != d->p_read;
}

void u8_rb_fifo_push(struct u8_ringbuf *d, uint8_t data)
{
    if (u8_rb_fifo_has_data(d)) {
        d->p_read++; // bye bye data
    }
    *(d->p_write++) = data;
    if (d->p_write >= d->p_end) {
        d->p_write = d->buffer;
    }
}

uint8_t u8_rb_fifo_pop(struct u8_ringbuf *d)
{
    uint8_t ret = *(d->p_read++);
    if (d->p_read >= d->p_end) {
        d->p_read = d->buffer;
    }
    return ret;
}

void u8_rb_fifo_init(struct u8_ringbuf *d)
{
    memset(d->buffer, 0, RINGBUF_SIZE);
    d->p_end = d->buffer + RINGBUF_SIZE;
    d->p_write = d->buffer;
    d->p_read = d->buffer;
}
