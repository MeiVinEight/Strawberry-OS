#ifndef __KERNEL_8254_H__
#define __KERNEL_8254_H__

#include <types.h>

#define PIT_FREQUENCY 1193180
#define PIT0_DATA 0x40
#define PIT1_DATA 0x41
#define PIT2_DATA 0x42
#define PIT_CMD   0x43
#define PIT2_GATE 0x61

void setup_8254(DWORD);

#endif