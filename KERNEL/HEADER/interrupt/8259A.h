#ifndef __KERNEL_8259A_H__
#define __KERNEL_8259A_H__

#include <interrupt/interrupt.h>
#include <types.h>

#define PIC1_COMMAND 0x20 // IO base address for master PIC
#define PIC2_COMMAND 0xA0 // IO base address for slave PIC
#define PIC1_DATA    0x21
#define PIC2_DATA    0xA1
#define PIC1_OFFSET  IRQ_INT     // INT Num for master PIC
#define PIC2_OFFSET  IRQ_INT + 8 // INT Num for slave PIC

#define ICW0_EOI     0x20 // End of interrupt command code

#define ICW1_ICW4       0x01 /* Indicates that ICW4 will be present */
#define ICW1_SINGLE     0x02 /* Single (cascade) mode */
#define ICW1_INTERVAL4  0x04 /* Call address interval 4 (8) */
#define ICW1_LEVEL      0x08 /* Level triggered (edge) mode */
#define ICW1_INIT       0x10 /* Initialization - required! */
#define ICW1_DISABLE    0xFF // Disable PIC

#define ICW4_8086       0x01 /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO       0x02 /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE  0x08 /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode/master */
#define ICW4_SFNM       0x10 /* Special fully nested (not) */

void setup_8259A();
void disable_8259A();
void eoi_8259A(BYTE);

#endif