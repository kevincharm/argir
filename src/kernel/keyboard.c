#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <kernel/io.h>
#include <kernel/keyboard.h>

#define DEQUE_BUFSIZ   (512)

struct deque {
    char buffer[DEQUE_BUFSIZ];
    size_t head;
    size_t tail;
    size_t len;
};

static struct deque keybuf;
static struct deque *kbuf = &keybuf;

bool irq_test = false;

static inline void lifo_push(struct deque *d, char data)
{
    *(d->buffer + d->head) = data;
    d->head = (d->head + 1) % DEQUE_BUFSIZ;
    d->len += 1;
}

static inline char lifo_pop(struct deque *d)
{
    // Needs to be atomic wrt IRQs
    char ret = *(d->buffer + d->tail);
    d->tail = (d->tail + 1) % DEQUE_BUFSIZ;
    d->len -= 1;
    return ret;
}

/*static char kb_scan()
{
    char ret;
    do {
        ret = inb(IRQ_PORT_STATUS);
    } while (!(ret & 1));
    ret = (char)inb(IRQ_SCAN_CODE);
    return ret;
}*/

static void kb_scan_and_push()
{
    char ret = inb(IRQ_SCAN_CODE);
    if (ret > 0) {
        lifo_push(kbuf, ret);
    }
}

void keyboard_irq_handler()
{
    irq_test = true;
    kb_scan_and_push();
}

void keyboard_init()
{
    // TODO: memset yadda yadda
    kbuf->head = 0;
    kbuf->tail = 0;
    kbuf->len = 0;
}

void keyboard_main()
{
    // putchar(kb_scan());
    if (irq_test) {
        irq_test = false;
        putchar('!');
    }

    while (kbuf->len) {
        putchar(lifo_pop(kbuf));
    }
}
