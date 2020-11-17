#ifndef __ARGIR__PIC_H
#define __ARGIR__PIC_H

/**
 *  Programmable Interrupt Controller (8259)
 */
#define PIC1_PORT_CMD (0x20)
#define PIC1_PORT_DATA (PIC1_PORT_CMD + 1)
#define PIC2_PORT_CMD (0xA0)
#define PIC2_PORT_DATA (PIC2_PORT_CMD + 1)

void pic_eoi(unsigned int int_no);
void pic_remap();
void pic_irq_off(unsigned int irq_no);
void pic_irq_on(unsigned int irq_no);
void pic_enable_only_keyboard(void);
void pic_enable_all_irqs(void);

#endif /* __ARGIR__PIC_H */
