#ifndef __KERNEL_SYSTEM_H__
#define __KERNEL_SYSTEM_H__

#include <types.h>

#define SYSTEM_LINEAR 0xFFFF800000000000ULL

typedef struct _OS_SYSTEM_TABLE
{
	QWORD FONT;
	QWORD MMAP;
	QWORD BLAT;
	QWORD SCRN;
	QWORD RSDP;
} OS_SYSTEM_TABLE;

extern OS_SYSTEM_TABLE SYSTEM_TABLE;
extern BYTE CORE_LOCK;

void setup_system_table(OS_SYSTEM_TABLE *);
void SetupCPU();
void OutputCPU();

#endif//__KERNEL_SYSTEM_H__
