#ifndef __ARGIR__TERMINAL_H
#define __ARGIR__TERMINAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct terminal {
    size_t row;
    size_t col;
    size_t width;
    size_t height;
    uint8_t colour;
    uint16_t *buffer;
};

void terminal_scroll_up(struct terminal *term, size_t n);
void terminal_clear(struct terminal *term);
void terminal_write_char(struct terminal *term, const char str);
void terminal_write(struct terminal *term, const char *str);
void terminal_init(struct terminal *term);

#endif /* __ARGIR__TERMINAL_H */
