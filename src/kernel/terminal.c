#include <string.h>
#include <kernel/terminal.h>

#define VGA_TEXT_MODE_ADDR      ((volatile uint16_t *)0xb8000)
#define VGA_TEXT_MODE_WIDTH     (80u)
#define VGA_TEXT_MODE_HEIGHT    (25u)
#define VGA_COLOUR_BLACK        (0)
#define VGA_COLOUR_WHITE        (15)

struct terminal {
    size_t row;
    size_t col;
    size_t width;
    size_t height;
    uint8_t colour;
    volatile uint16_t *buffer;
};

static struct terminal term0;
static struct terminal *term = &term0;

static inline uint8_t vga_colour_calc(uint8_t fg, uint8_t bg)
{
    return fg | (bg << 4u);
}

static inline void terminal_vga_colour_set(uint8_t fg, uint8_t bg)
{
    term->colour = vga_colour_calc(fg, bg);
}

static inline void vga_text_set(size_t x, size_t y, unsigned char c)
{
    size_t i = (y * term->width) + x;
    term->buffer[i] = ((uint16_t)(term->colour) << 8u) | c;
}

void terminal_scroll_up(size_t n)
{
    size_t total = term->width * term->height;
    size_t sub = n * term->width;
    for (size_t i=sub; i<total; i++) {
        term->buffer[i-sub] = term->buffer[i];
    }
    for (size_t i=total-sub; i<total; i++) {
        term->buffer[i] = ' ';
    }
}

void terminal_write_char(const char c)
{
    switch (c) {
    case 0xd: /* CR */
    case '\n':
        term->col = 0;
        term->row += 1;
        break;
    case '\t':
        term->col += 4;
        break;
    case 0x8: /* backspace */
        vga_text_set(term->col-1, term->row, ' ');
        if (term->col) term->col -= 1;
        break;
    default:
        vga_text_set(term->col, term->row, c);
        term->col += 1;
    }

    if (term->col >= term->width) {
        term->col = 0;
        term->row += 1;
    }
    if (term->row >= term->height) {
        terminal_scroll_up(1);
        term->row -= 1;
    }
}

void terminal_clear()
{
    for (size_t y=0; y<(term->height); y++) {
        for (size_t x=0; x<(term->width); x++) {
            vga_text_set(x, y, ' ');
        }
    }
}

void terminal_write(const char *str)
{
    size_t len = strlen(str);
    for (size_t i=0; i<len; i++) {
        terminal_write_char(str[i]);
    }
}

void terminal_init()
{
    term->row = 0;
    term->col = 0;
    term->width = VGA_TEXT_MODE_WIDTH;
    term->height = VGA_TEXT_MODE_HEIGHT;
    term->buffer = VGA_TEXT_MODE_ADDR;
    terminal_vga_colour_set(VGA_COLOUR_WHITE, VGA_COLOUR_BLACK);
    terminal_clear();
}
