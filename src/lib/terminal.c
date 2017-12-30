#include "terminal.h"

#define VGA_TEXT_MODE_ADDR      ((uint16_t *)0xb8000)
#define VGA_TEXT_MODE_WIDTH     (80u)
#define VGA_TEXT_MODE_HEIGHT    (25u)
#define VGA_COLOUR_BLACK        (0)
#define VGA_COLOUR_WHITE        (15)

size_t strlen(const char *str)
{
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

static inline uint8_t vga_colour_calc(uint8_t fg, uint8_t bg)
{
    return fg | (bg << 4u);
}

static inline void terminal_vga_colour_set(struct terminal *term, uint8_t fg, uint8_t bg)
{
    term->colour = vga_colour_calc(fg, bg);
}

static inline void vga_text_set(struct terminal *term, size_t x, size_t y, unsigned char c)
{
    size_t i = (y * term->width) + x;
    term->buf[i] = ((uint16_t)(term->colour) << 8u) | c;
}

void terminal_write_char(struct terminal *term, const char c)
{
    if (c == '\n') {
        term->col = 0;
        term->row += 1;
        return;
    } else {
        vga_text_set(term, term->col, term->row, c);
        term->col += 1;
    }

    if (term->col >= term->width) {
        term->col = 0;
        term->row += 1;
        if (term->row >= term->height) {
            term->row = 0;
        }
    }
}

void terminal_clear(struct terminal *term)
{
    for (size_t y=0; y<(term->height); y++) {
        for (size_t x=0; x<(term->width); x++) {
            vga_text_set(term, x, y, ' ');
        }
    }
}

void terminal_write(struct terminal *term, const char *str)
{
    size_t len = strlen(str);
    for (size_t i=0; i<len; i++) {
        terminal_write_char(term, str[i]);
    }
}

void terminal_init(struct terminal *term)
{
    term->row = 0;
    term->col = 0;
    term->width = VGA_TEXT_MODE_WIDTH;
    term->height = VGA_TEXT_MODE_HEIGHT;
    term->buf = VGA_TEXT_MODE_ADDR;
    terminal_vga_colour_set(term, VGA_COLOUR_WHITE, VGA_COLOUR_BLACK);
    terminal_clear(term);
}
