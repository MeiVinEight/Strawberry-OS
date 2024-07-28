#ifndef __KERNEL_DEVICE_KEYBOARD_H__
#define __KERNEL_DEVICE_KEYBOARD_H__

#include <types.h>

typedef struct _KEY_EVENT_RING
{
	BYTE  *RNG;
	WORD   CNT;
	WORD   NID;
	WORD   EID;
} KEY_EVENT_RING;

extern KEY_EVENT_RING KEY_RING;

void KeyEvent(KEY_EVENT_RING *, BYTE);
BYTE KeyNext(KEY_EVENT_RING *);
void CreateKeyEventRing(KEY_EVENT_RING *);

#endif