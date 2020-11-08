#include <string.h>
#include <kernel/terminal.h>

extern uint64_t KFONT_VGA_LEN;
extern uint64_t KFONT_VGA_WIDTH;
extern uint64_t KFONT_VGA_HEIGHT;
extern uint8_t KFONT_VGA[];
#define GLYPH_WIDTH (KFONT_VGA_LEN / KFONT_VGA_WIDTH)

struct terminal {
    size_t row;
    size_t col;
    size_t width;
    size_t height;
    uint8_t colour;
    /** Framebuffer */
    uint64_t *fb;
    size_t fb_pitch;
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
    // term->buffer[i] = ((uint16_t)(term->colour) << 8u) | c;
    // Find the glyph offset in the font bitmap
    size_t glyph_x_off = (c - 0x20) * GLYPH_WIDTH;
    // Copy pixels from glyph to framebuffer
    for (size_t j = 0; j < KFONT_VGA_HEIGHT; j++) {
        for (size_t i = 0; i < GLYPH_WIDTH; i++) {
            size_t index = KFONT_VGA_WIDTH * j + glyph_x_off + i;
            uint32_t argb = ((KFONT_VGA[index] << 16) & 0xff0000) |
                            ((KFONT_VGA[index + 1] << 8) & 0xff00) |
                            KFONT_VGA[index + 2];

            // Calculate framebuffer offset, i.e. where to copy pixels to
            size_t fb_off = term->fb_pitch * (KFONT_VGA_HEIGHT * y + j) +
                            (GLYPH_WIDTH * x + i) * 4;
            *((uint32_t *)(term->fb + fb_off)) = argb;
        }
    }
}

void terminal_scroll_up(size_t n)
{
    // size_t total = term->width * term->height;
    // size_t sub = n * term->width;
    // for (size_t i = sub; i < total; i++) {
    //     term->buffer[i - sub] = term->buffer[i];
    // }
    // for (size_t i = total - sub; i < total; i++) {
    //     term->buffer[i] = ' ';
    // }
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
    for (size_t y = 0; y < (term->height); y++) {
        for (size_t x = 0; x < (term->width); x++) {
            vga_text_set(x, y, ' ');
        }
    }
}

void terminal_write(const char *str)
{
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++) {
        terminal_write_char(str[i]);
    }
}

void terminal_init(uint64_t *framebuffer, size_t screen_width,
                   size_t screen_height, size_t fb_scanline)
{
    term->row = 0;
    term->col = 0;
    term->width = screen_width / GLYPH_WIDTH;
    term->height = screen_height / KFONT_VGA_HEIGHT;
    term->fb = framebuffer;
    term->fb_pitch = fb_scanline;
    // terminal_clear();
}
