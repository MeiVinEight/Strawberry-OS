#ifndef __KERNEL_INTRINSIC_H__
#define __KERNEL_INTRINSIC_H__

#include <types.h>

void  __cpuid(int *, int);
void  __cpuidex(int *, int, int);
void  __halt();
BYTE  __inbyte(DWORD);
DWORD __indword(DWORD);
WORD  __inword(DWORD);
void  __lidt(void *);
void  __nop();
void  __outbyte(DWORD, BYTE);
void  __outdword(DWORD, DWORD);
void  __outword(DWORD, WORD);
QWORD __rdtsc();
QWORD __readcr0(void);
QWORD __readcr2(void);
QWORD __readcr3(void);
QWORD __readcr4(void);
QWORD __readmsr(DWORD);
WORD  __reverse16(WORD);
DWORD __reverse32(DWORD);
QWORD __reverse64(QWORD);
void  __sidt(void *);
void  __writecr0(QWORD);
void  __writecr4(QWORD);
void  __writemsr(DWORD, QWORD);

void __setrsp(QWORD);
QWORD __getrsp();
void __cli();
void __sti();
void *memset(void *, DWORD, QWORD);
void *memsetq(void *, QWORD, QWORD);
void *memcpy(void *, const void *, QWORD);
void *memcpyq(void *, const void *, QWORD);
void __lgdt(void *);

#endif