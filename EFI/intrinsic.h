#ifndef __INTRINSIC_H__
#define __INTRINSIC_H__

void __halt();
QWORD __readcr3(void);
void __lidt(void *);
void *memset(void *, int, unsigned long long);
void *__cdecl memcpy(void *, const void *, unsigned long long);
void __cli();
void __setrsp(unsigned long long);
QWORD __lgdt();

#endif