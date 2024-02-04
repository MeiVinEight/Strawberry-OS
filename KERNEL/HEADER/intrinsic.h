#ifndef __KERNEL_INTRINSIC_H__
#define __KERNEL_INTRINSIC_H__

#include <types.h>

void __halt();
void __outbyte(DWORD, BYTE);
void __lidt(void *);
void __sidt(void *);
QWORD __readcr2(void);

void __setrsp(QWORD);
QWORD __getrsp();
void __cli();
void __sti();
void *memset(void *, DWORD, QWORD);

#endif