#ifndef __ARGIR__KEYBOARD_H
#define __ARGIR__KEYBOARD_H

#define PS2_PORT_DATA       (0x60)      /* R/W: Data */
#define PS2_PORT_STATCMD    (0x64)      /* R: status, W: command */

void keyboard_irq_handler();
void keyboard_init();
void keyboard_main();

#endif /* __ARGIR__KEYBOARD_H */
