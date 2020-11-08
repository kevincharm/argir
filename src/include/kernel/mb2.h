#ifndef __ARGIR__MB2_H
#define __ARGIR__MB2_H

#include <stdint.h>

struct mb2_tag {
    uint32_t type;
    uint32_t size;
    union {
        struct mb2_tag_fb {
            uint64_t framebuffer_addr;
            uint32_t framebuffer_pitch;
            uint32_t framebuffer_width;
            uint32_t framebuffer_height;
            uint8_t framebuffer_bpp;
            uint8_t framebuffer_type;
            uint8_t reserved;
        } __attribute__((packed)) framebuffer;
    };
} __attribute__((packed));

struct mb2_info {
    uint32_t total_size;
    uint32_t reserved;
} __attribute__((packed));

struct mb2_tag *mb2_find_tag(struct mb2_info *mb2_info, uint32_t type);

#endif /* __ARGIR__MB2_H */
