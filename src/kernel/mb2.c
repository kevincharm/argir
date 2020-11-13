#include <stddef.h>
#include "kernel/mb2.h"

struct mb2_tag *mb2_find_tag(uint32_t mb2_info, uint32_t type)
{
    for (struct mb2_tag *tag = (struct mb2_tag *)(mb2_info + 8); tag->type != 0;
         tag = (struct mb2_tag *)((uint8_t *)tag + ((tag->size + 7) & ~7))) {
        if (tag->type == type) {
            return tag;
        }
    }

    return NULL;
}