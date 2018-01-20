#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <kernel/io.h>
#include <kernel/irq.h>
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
        ret = inb(PS2_PORT_STATCMD);
    } while (!(ret & 1));
    ret = (char)inb(PS2_PORT_DATA);
    return ret;
}*/

static void kb_scan_and_push()
{
    char ret = inb(PS2_PORT_DATA);
    if (ret > 0) {
        lifo_push(kbuf, ret);
    }
}

void keyboard_irq_handler()
{
    irq_test = true;
    kb_scan_and_push();
}

static void ps2_wait_cmd(uint16_t port, uint8_t data)
{
    uint8_t ret;
    do {
        ret = inb(PS2_PORT_STATCMD);
    } while (ret & 0x02);
    outb(port, data);
}

static void ps2_flush_output_buf()
{
    uint8_t ret;
    do {
        ret = inb(PS2_PORT_DATA);
    } while (ret & 0x01);
}

void keyboard_init()
{
    // TODO: memset yadda yadda
    kbuf->head = 0;
    kbuf->tail = 0;
    kbuf->len = 0;

    // 8042 initialisation
    uint8_t ret;

    // Disable first PS/2 port
    outb(PS2_PORT_STATCMD, 0xad);

    // Flush output buffer
    ps2_flush_output_buf();

    // Read config
    ps2_wait_cmd(PS2_PORT_STATCMD, 0x20);
    ret = inb(PS2_PORT_DATA);
    uint8_t config = ret;
    config &= ~(1u << 0u); /* PS/2 IRQ1 */
    config &= ~(1u << 1u); /* PS/2 IRQ12 */
    config &= ~(1u << 6u); /* Translation */
    // Write config
    ps2_wait_cmd(PS2_PORT_STATCMD, 0x60);
    ps2_wait_cmd(PS2_PORT_DATA, config);

    // Self-test
    ps2_wait_cmd(PS2_PORT_STATCMD, 0xaa);
    ret = inb(PS2_PORT_DATA);
    if (ret != 0x55) {
        printf("8042: self-test failed!\n");
    }

    // Test first PS/2 port
    ps2_wait_cmd(PS2_PORT_STATCMD, 0xab);
    ret = inb(PS2_PORT_DATA);
    switch (ret) {
    case 0x00:
        break;
    case 0x01:
        printf("8042: PS/2 #1 clock line stuck low!\n");
        break;
    case 0x02:
        printf("8042: PS/2 #1 clock line stuck high!\n");
        break;
    case 0x03:
        printf("8042: PS/2 #1 data line stuck low!\n");
        break;
    case 0x04:
        printf("8042: PS/2 #1 data line stuck high!\n");
        break;
    default:
        printf("8042: PS/2 #1 unknown error!\n");
    }

    // Enable first PS/2 port
    ps2_wait_cmd(PS2_PORT_STATCMD, 0xae);

    // Read config
    ps2_wait_cmd(PS2_PORT_STATCMD, 0x20);
    ret = inb(PS2_PORT_DATA);
    config = ret;
    config |= (1u << 0u); /* bit 0 set -> enable PS/2 #1 int */
    config |= (1u << 4u);
    // Write config
    ps2_wait_cmd(PS2_PORT_STATCMD, 0x60);
    ps2_wait_cmd(PS2_PORT_DATA, config);

    // Enable scanning
    ps2_wait_cmd(PS2_PORT_DATA, 0xf4);
    ret = inb(PS2_PORT_DATA);
    if (ret != 0xfa) {
        printf("PS/2 enable scanning failed!\n");
    }

    // Reset PS/2 device
    ps2_wait_cmd(PS2_PORT_DATA, 0xff);
    ret = inb(PS2_PORT_DATA);
    if (ret != 0xfa) {
        printf("PS/2 reset failed!\n");
    }

    printf("Keyboard initialised!\n\n");
}

void keyboard_main()
{
    // putchar(kb_scan());
    if (irq_test) {
        irq_test = false;
        printf("a");
    }

    while (kbuf->len) {
        putchar(lifo_pop(kbuf));
    }
}
