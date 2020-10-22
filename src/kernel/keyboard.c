#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <memory.h>
#include <ringbuf.h>
#include <kernel/io.h>
#include <kernel/keyboard.h>

#define KB_SCAN2_BREAK (0xf0) /* TODO: Put in keycode map */

#define KB_NUL (0)
#define KB_FKEYS (0x100)
#define KB_ESCAPE (KB_FKEYS + 0)
#define KB_F1 (KB_FKEYS + 1)
#define KB_F2 (KB_FKEYS + 2)
#define KB_F3 (KB_FKEYS + 3)
#define KB_F4 (KB_FKEYS + 4)
#define KB_F5 (KB_FKEYS + 5)
#define KB_F6 (KB_FKEYS + 6)
#define KB_F7 (KB_FKEYS + 7)
#define KB_F8 (KB_FKEYS + 8)
#define KB_F9 (KB_FKEYS + 9)
#define KB_F10 (KB_FKEYS + 10)
#define KB_F11 (KB_FKEYS + 11)
#define KB_F12 (KB_FKEYS + 12)
#define KB_MODKEYS (0x110)
#define KB_LALT (KB_MODKEYS + 0)
#define KB_RALT (KB_MODKEYS + 1)
#define KB_LSHIFT (KB_MODKEYS + 2)
#define KB_RSHIFT (KB_MODKEYS + 3)
#define KB_LCTRL (KB_MODKEYS + 4)
#define KB_RCTRL (KB_MODKEYS + 5)
#define KB_CAPSLOCK (KB_MODKEYS + 6)
#define KB_NUMLOCK (KB_MODKEYS + 7)
#define KB_SCRLOCK (KB_MODKEYS + 8)
#define KB_ENTER (0xd)
#define KB_BACKSPACE (0x8)

static uint32_t kb_ps2_scancode2[] = {
    KB_NUL, KB_F9,  KB_NUL, KB_F5,     KB_F3,        KB_F1,     KB_F2,
    KB_F12, KB_NUL, KB_F10, KB_F8,     KB_F6,        KB_F4,     '\t',
    '`',    KB_NUL, KB_NUL, KB_LALT,   KB_LSHIFT,    KB_NUL,    KB_LCTRL,
    'q',    '1',    KB_NUL, KB_NUL,    KB_NUL,       'z',       's',
    'a',    'w',    '2',    KB_NUL,    KB_NUL,       'c',       'x',
    'd',    'e',    '4',    '3',       KB_NUL,       KB_NUL,    ' ',
    'v',    'f',    't',    'r',       '5',          KB_NUL,    KB_NUL,
    'n',    'b',    'h',    'g',       'y',          '6',       KB_NUL,
    KB_NUL, KB_NUL, 'm',    'j',       'u',          '7',       '8',
    KB_NUL, KB_NUL, ',',    'k',       'i',          'o',       '0',
    '9',    KB_NUL, KB_NUL, '.',       '/',          'l',       ';',
    'p',    '-',    KB_NUL, KB_NUL,    KB_NUL,       '\'',      KB_NUL,
    '[',    '=',    KB_NUL, KB_NUL,    KB_CAPSLOCK,  KB_RSHIFT, KB_ENTER,
    ']',    KB_NUL, '\\',   KB_NUL,    KB_NUL,       KB_NUL,    KB_NUL,
    KB_NUL, KB_NUL, KB_NUL, KB_NUL,    KB_BACKSPACE, KB_NUL,    KB_NUL,
    '1',    KB_NUL, '4', /* This line and below are numpad */
    '7',    KB_NUL, KB_NUL, KB_NUL,    '0',          '.',       '2',
    '5',    '6',    '8',    KB_ESCAPE, KB_NUMLOCK,   KB_F11,    '+',
    '3',    '-',    '*',    '9',       KB_SCRLOCK,   KB_NUL,    KB_NUL,
    KB_NUL, KB_NUL, KB_F7
};

static uint32_t kb_ps2_scancode2_shift[] = {
    KB_NUL, KB_F9,  KB_NUL, KB_F5,     KB_F3,        KB_F1,     KB_F2,
    KB_F12, KB_NUL, KB_F10, KB_F8,     KB_F6,        KB_F4,     '\t',
    '~',    KB_NUL, KB_NUL, KB_LALT,   KB_LSHIFT,    KB_NUL,    KB_LCTRL,
    'Q',    '!',    KB_NUL, KB_NUL,    KB_NUL,       'Z',       'S',
    'A',    'W',    '@',    KB_NUL,    KB_NUL,       'C',       'X',
    'D',    'E',    '$',    '#',       KB_NUL,       KB_NUL,    ' ',
    'V',    'F',    'T',    'R',       '%',          KB_NUL,    KB_NUL,
    'N',    'B',    'H',    'G',       'Y',          '^',       KB_NUL,
    KB_NUL, KB_NUL, 'M',    'J',       'U',          '&',       '*',
    KB_NUL, KB_NUL, '<',    'K',       'I',          'O',       ')',
    '(',    KB_NUL, KB_NUL, '>',       '?',          'L',       ':',
    'P',    '_',    KB_NUL, KB_NUL,    KB_NUL,       '"',       KB_NUL,
    '{',    '+',    KB_NUL, KB_NUL,    KB_CAPSLOCK,  KB_RSHIFT, KB_ENTER,
    '}',    KB_NUL, '|',    KB_NUL,    KB_NUL,       KB_NUL,    KB_NUL,
    KB_NUL, KB_NUL, KB_NUL, KB_NUL,    KB_BACKSPACE, KB_NUL,    KB_NUL,
    '1',    KB_NUL, '4', /* This line and below are numpad */
    '7',    KB_NUL, KB_NUL, KB_NUL,    '0',          '.',       '2',
    '5',    '6',    '8',    KB_ESCAPE, KB_NUMLOCK,   KB_F11,    '+',
    '3',    '-',    '*',    '9',       KB_SCRLOCK,   KB_NUL,    KB_NUL,
    KB_NUL, KB_NUL, KB_F7
};

static struct u8_ringbuf keybuf;
static struct u8_ringbuf *kbuf = &keybuf;

static volatile bool break_next = false;
static volatile bool shift_next = false;
static volatile bool caps_lock = false;

void keyboard_irq_handler()
{
    uint8_t code = inb(PS2_PORT_DATA);

    if (code == KB_SCAN2_BREAK) {
        break_next = true;
        goto done;
    }

    // Check OOB
    if (code * sizeof(*kb_ps2_scancode2) > sizeof(kb_ps2_scancode2)) {
        goto done;
    }

    uint32_t key;
    if (shift_next) {
        key = kb_ps2_scancode2_shift[code];
    } else {
        key = kb_ps2_scancode2[code];
    }

    if (key == KB_LSHIFT || key == KB_RSHIFT) {
        if (!break_next) {
            shift_next = true;
        } else {
            shift_next = false;
        }
        goto input_finished;
    }

    if (key == KB_CAPSLOCK) {
        if (break_next) {
            caps_lock = !caps_lock;
        }
        goto input_finished;
    }

    if ((key >= 0x20 && key <= 0x7f) || key == KB_BACKSPACE ||
        key == KB_ENTER) {
        // Printable ASCII
        if (caps_lock && key >= 0x61 && key <= 0x7a) {
            // Transform lowercase->uppercase
            key -= 0x20;
        }
        if (!break_next) {
            u8_rb_fifo_push(kbuf, key & 0xff);
        }
        goto input_finished;
    }

input_finished:
    if (break_next) {
        break_next = false;
    }

done:
    return;
}

static void ps2_wait_write(uint16_t port, uint8_t data)
{
    uint8_t ret;
    do {
        ret = inb(PS2_PORT_STATCMD);
    } while (ret & 0x02);
    outb(port, data);
}

static uint8_t ps2_wait_read(uint16_t port)
{
    uint8_t ret;
    do {
        ret = inb(PS2_PORT_STATCMD);
    } while (!(ret & 0x01));
    return inb(port);
}

static void ps2_flush_output()
{
    uint8_t ret;
    do {
        inb(PS2_PORT_DATA);
        ret = inb(PS2_PORT_STATCMD);
    } while (ret & 0x01);
}

void keyboard_init()
{
    u8_rb_fifo_init(kbuf);

    // 8042 initialisation
    uint8_t ret;

    // Disable PS/2 ports
    outb(PS2_PORT_STATCMD, 0xad);
    outb(PS2_PORT_STATCMD, 0xa7);

    // Flush output buffer
    ps2_flush_output();

    // Read config
    ps2_wait_write(PS2_PORT_STATCMD, 0x20);
    uint8_t config = ps2_wait_read(PS2_PORT_DATA);
    config &= ~(1u << 0u); /* PS/2 IRQ1 */
    config &= ~(1u << 1u); /* PS/2 IRQ12 */
    config &= ~(1u << 6u); /* Translation */
    // Write config
    ps2_wait_write(PS2_PORT_STATCMD, 0x60);
    ps2_wait_write(PS2_PORT_DATA, config);

    // Self-test
    ps2_wait_write(PS2_PORT_STATCMD, 0xaa);
    ret = ps2_wait_read(PS2_PORT_DATA);
    if (ret != 0x55) {
        printf("8042: self-test failed!\n");
    }

    // Test first PS/2 port
    ps2_wait_write(PS2_PORT_STATCMD, 0xab);
    ret = ps2_wait_read(PS2_PORT_DATA);
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
    ps2_wait_write(PS2_PORT_STATCMD, 0xae);

    // Read config
    ps2_wait_write(PS2_PORT_STATCMD, 0x20);
    ret = ps2_wait_read(PS2_PORT_DATA);
    config = ret;
    config |= (1u << 0u); /* IRQ1 */
    config &= ~(1u << 4u); /* Enable PS/2 port clock */
    config &= ~(1u << 6u); /* Translation */
    config &= ~(1u << 7u); /* Must be zero */
    // Write config
    ps2_wait_write(PS2_PORT_STATCMD, 0x60);
    ps2_wait_write(PS2_PORT_DATA, config);

    // Set autorepeat -> 500ms
    ps2_wait_write(PS2_PORT_DATA, 0xf3);
    ps2_wait_write(PS2_PORT_DATA, 0x20);

    // Set scancode -> set 2
    ps2_wait_write(PS2_PORT_DATA, 0xf0);
    ps2_wait_write(PS2_PORT_DATA, 0x02);

    // Flags
    ps2_wait_write(PS2_PORT_DATA, 0xfa);

    // Enable scanning
    ps2_wait_write(PS2_PORT_DATA, 0xf4);
    ret = ps2_wait_read(PS2_PORT_DATA);
    if (ret != 0xfa) {
        printf("PS/2 enable scanning failed!\n");
    }

    // Reset PS/2 device
    ps2_wait_write(PS2_PORT_DATA, 0xff);
    ret = ps2_wait_read(PS2_PORT_DATA);
    if (ret != 0xfa) {
        printf("PS/2 reset failed!\n");
    }

    ps2_flush_output();

    printf("Initialised keyboard.\n");
}

void keyboard_main()
{
    while (u8_rb_fifo_has_data(kbuf)) {
        putchar(u8_rb_fifo_pop(kbuf));
    }
}
