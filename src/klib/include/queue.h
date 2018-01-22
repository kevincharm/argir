#ifndef _QUEUE_H
#define _QUEUE_H

#include <stddef.h>
#include <stdint.h>

/* DEQUE */
#define DEQUE_BUFSIZ   (512)

struct u8_deque {
    uint8_t buffer[DEQUE_BUFSIZ];
    size_t head;
    size_t tail;
    size_t len;
};

void u8_lifo_init(struct u8_deque *d);
void u8_lifo_push(struct u8_deque *d, uint8_t data);
uint8_t u8_lifo_pop(struct u8_deque *d);

#endif /* _QUEUE_H */
