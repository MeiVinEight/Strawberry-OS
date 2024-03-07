#ifndef __KERNEL_SYSTEM_H__
#define __KERNEL_SYSTEM_H__

#include <types.h>

typedef struct _OS_SYSTEM_TABLE
{
	QWORD FONT;
	QWORD MMAP;
	QWORD BLAT;
	QWORD PTME;
	QWORD PAGE;
	QWORD SCRN;
	QWORD RSDP;
} OS_SYSTEM_TABLE;

extern OS_SYSTEM_TABLE OST;

void setup_system_table(OS_SYSTEM_TABLE *);

#endif//__KERNEL_SYSTEM_H__
