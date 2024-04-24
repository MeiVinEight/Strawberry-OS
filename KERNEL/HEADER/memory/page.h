#ifndef __KERNEL_PAGE_H__
#define __KERNEL_PAGE_H__

#include <types.h>

#define PAGE4_4K 0
#define PAGE4_2M 1
#define PAGE4_1G 2

void setup_paging();
DWORD linear_mapping(QWORD, QWORD, BYTE, QWORD);
DWORD linear_unmapping(QWORD);
DWORD identity_mapping(QWORD, BYTE);
QWORD physical_mapping(QWORD);
DWORD AllocatePhysicalMemory(QWORD *, QWORD, QWORD *);
DWORD FreePhysicalMemory(QWORD, QWORD, QWORD);
void MemoryNotEnough(DWORD);

#endif