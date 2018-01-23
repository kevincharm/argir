#include <ringbuf.h>
#include <memory.h>

bool u8_rb_fifo_has_data(struct u8_ringbuf *rb)
{
    return rb->p_write != rb->p_read;
}

void u8_rb_fifo_push(struct u8_ringbuf *rb, uint8_t data)
{
    if (u8_rb_fifo_has_data(rb)) {
        rb->p_read++; // bye bye data
    }
    *(rb->p_write++) = data;
    if (rb->p_write >= rb->p_end) {
        rb->p_write = rb->buffer;
    }
}

uint8_t u8_rb_fifo_pop(struct u8_ringbuf *rb)
{
    uint8_t ret = *(rb->p_read++);
    if (rb->p_read >= rb->p_end) {
        rb->p_read = rb->buffer;
    }
    return ret;
}

void u8_rb_fifo_init(struct u8_ringbuf *rb)
{
    memset(rb->buffer, 0, RINGBUF_SIZE);
    rb->p_end = rb->buffer + RINGBUF_SIZE;
    rb->p_write = rb->buffer;
    rb->p_read = rb->buffer;
}
