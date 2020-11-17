#ifndef __ARGIR__MB2_H
#define __ARGIR__MB2_H

#include <stdint.h>

struct mb2_memory_map_entry {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} __attribute__((packed));

#define MB_TAG_TYPE_TERMINATOR (0)
#define MB_TAG_TYPE_MEMORY_MAP (6)
#define MB_TAG_TYPE_FRAMEBUFFER (8)

struct mb2_tag {
    uint32_t type;
    uint32_t size;
    union {
        struct mb2_tag_fb {
            uint64_t addr;
            uint32_t pitch;
            uint32_t width;
            uint32_t height;
            uint8_t bpp;
            uint8_t fb_type;
            uint8_t reserved;
        } __attribute__((packed)) framebuffer;

        struct mb2_tag_memory_map {
            uint32_t entry_size;
            uint32_t entry_version;
            struct mb2_memory_map_entry *entries;
        } __attribute__((packed)) memory_map;
    };
} __attribute__((packed));

struct mb2_info {
    uint32_t total_size;
    uint32_t reserved;
} __attribute__((packed));

struct mb2_tag *mb2_find_tag(uint32_t mb2_info, uint32_t type);

#endif /* __ARGIR__MB2_H */
