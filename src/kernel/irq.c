#include <stddef.h>
#include <kernel/irq.h>

struct idt_entry idt[256];

void irq_init()
{
    // TODO: replace initialisation with memset
    for (size_t i=0; i<256; i++) {
        uint8_t *u8idt = (void *)(idt+i);
        for (size_t j=0; j<3; j++)
            u8idt[j] = 0;
    }

    // TODO: set IDT entries

    lidt(&idt, (sizeof (struct idt_entry) * 256) - 1);
}
