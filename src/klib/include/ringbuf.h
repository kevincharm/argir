#ifndef _RINGBUF_H
#define _RINGBUF_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define RINGBUF_SIZE (512)

struct u8_ringbuf {
    uint8_t buffer[RINGBUF_SIZE];
    uint8_t *p_write;
    uint8_t *p_read;
    uint8_t *p_end;
};

bool u8_rb_fifo_has_data(struct u8_ringbuf *rb);
void u8_rb_fifo_init(struct u8_ringbuf *rb);
void u8_rb_fifo_push(struct u8_ringbuf *rb, uint8_t data);
uint8_t u8_rb_fifo_pop(struct u8_ringbuf *rb);

#endif /* _RINGBUF_H */
