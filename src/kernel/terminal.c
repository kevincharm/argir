#include <string.h>
#include "kernel/terminal.h"

extern uint64_t KFONT_VGA_LEN;
extern uint64_t KFONT_VGA_WIDTH;
extern uint64_t KFONT_VGA_HEIGHT;
extern uint8_t KFONT_VGA[];
#define GLYPH_WIDTH (KFONT_VGA_LEN / KFONT_VGA_WIDTH)

struct colour {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

struct terminal {
    size_t row;
    size_t col;
    size_t width;
    size_t height;
    struct colour fg_colour;
    struct colour bg_colour;
    size_t scale;
    /** Framebuffer */
    volatile uint8_t *fb;
    size_t screen_width; // pixels
    size_t screen_height; // pixels
    size_t fb_pitch; // bytes
};

static struct terminal term0;
static struct terminal *term = &term0;

static inline void vga_text_set(size_t x, size_t y, unsigned char c)
{
    if (term->fb == NULL)
        return;

    // Find the glyph offset in the font bitmap
    size_t glyph_x_off = (c - 0x20) * GLYPH_WIDTH;
    // Copy pixels from glyph to framebuffer
    for (size_t j = 0; j < KFONT_VGA_HEIGHT * term->scale; j++) {
        for (size_t i = 0; i < GLYPH_WIDTH * term->scale; i++) {
            size_t index = (KFONT_VGA_WIDTH * (j / term->scale) + glyph_x_off +
                            (i / term->scale));
            index -= 1; // idk, font file issue?

            uint8_t red, green, blue, alpha;
            bool is_fg =
                KFONT_VGA[index] + KFONT_VGA[index + 1] + KFONT_VGA[index + 2] >
                0;
            if (is_fg) {
                red = KFONT_VGA[index] * term->fg_colour.r / 0xff;
                green = KFONT_VGA[index + 1] * term->fg_colour.g / 0xff;
                blue = KFONT_VGA[index + 2] * term->fg_colour.b / 0xff;
                alpha = term->fg_colour.a;
            } else {
                red = term->bg_colour.r;
                green = term->bg_colour.g;
                blue = term->bg_colour.b;
                alpha = term->bg_colour.a;
            }
            uint32_t argb = ((alpha << 24) | 0xff000000) |
                            ((red << 16) & 0xff0000) | ((green << 8) & 0xff00) |
                            blue;

            // Calculate framebuffer offset, i.e. where to copy pixels to
            size_t fb_off =
                term->fb_pitch * (KFONT_VGA_HEIGHT * term->scale * y + j) +
                (GLYPH_WIDTH * term->scale * x + i) * 4;
            *((uint32_t *)((uint8_t *)term->fb + fb_off)) = argb;
        }
    }
}

void terminal_scroll_up(size_t n)
{
    if (term->fb == NULL)
        return;

    size_t height_px = term->screen_height;
    size_t pitch = term->fb_pitch;
    size_t cut = KFONT_VGA_HEIGHT * term->scale * n;
    for (size_t j = 0; j < height_px - cut; j++) {
        size_t j_2 = j + cut;
        memcpy(term->fb + (j * pitch), term->fb + (j_2 * pitch), pitch);
    }
    for (size_t j = height_px - cut; j < height_px; j++) {
        size_t j_2 = j + cut;
        memset(term->fb + (j * pitch), 0, pitch);
    }
}

void terminal_set_fg_colour(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    term->fg_colour.r = r;
    term->fg_colour.g = g;
    term->fg_colour.b = b;
    term->fg_colour.a = a;
}

void terminal_set_bg_colour(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    term->bg_colour.r = r;
    term->bg_colour.g = g;
    term->bg_colour.b = b;
    term->bg_colour.a = a;
}

void terminal_write_char(const char c)
{
    switch (c) {
    case '\r':
        term->col = 0;
        break;
    case '\n':
        term->col = 0;
        term->row += 1;
        break;
    case '\t':
        term->col += 4;
        break;
    case 0x8: /* backspace */
        vga_text_set(term->col - 1, term->row, ' ');
        if (term->col)
            term->col -= 1;
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
    for (size_t y = 0; y < term->height; y++) {
        for (size_t x = 0; x < term->width; x++) {
            vga_text_set(x, y, ' ');
        }
    }
    term->row = 0;
    term->col = 0;
}

void terminal_write(const char *str)
{
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++) {
        terminal_write_char(str[i]);
    }
}

void terminal_init(uint64_t *framebuffer, size_t screen_width,
                   size_t screen_height, size_t fb_scanline, size_t scale)
{
    term->row = 0;
    term->col = 0;
    term->scale = 2;
    term->fg_colour.r = 0xff;
    term->fg_colour.g = 0xff;
    term->fg_colour.b = 0xff;
    term->fg_colour.a = 0xff;
    term->bg_colour.r = 0;
    term->bg_colour.g = 0;
    term->bg_colour.b = 0;
    term->bg_colour.a = 0;
    term->width = screen_width / (GLYPH_WIDTH * term->scale);
    term->height = screen_height / (KFONT_VGA_HEIGHT * term->scale);
    term->fb = framebuffer;
    term->screen_width = screen_width;
    term->screen_height = screen_height;
    term->fb_pitch = fb_scanline;
    terminal_clear();
}
