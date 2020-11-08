#include <stddef.h>
#include "kernel/mb2.h"

struct mb2_tag *mb2_find_tag(struct mb2_info *mb2_info, uint32_t type)
{
    uint32_t *tag_base = mb2_info + sizeof(struct mb2_info);
    uint32_t *tag_limit = tag_base + mb2_info->total_size;
    struct mb2_tag *tag = NULL;

    while (tag_base < tag_limit) {
        if (tag->type == type) {
            goto done;
        }

        // increment by tag size, then align to 8 bytes
        tag_base += tag->size + (tag->size % 8);
    }

done:
    return tag;
}