#ifndef __KERNEL_TIMER_H__
#define __KERNEL_TIMER_H__

#include <types.h>

#define TIMER_INT_FREQUENCY 20

void setup_timer();
void delay(QWORD);
QWORD TimestampCPU();

#endif