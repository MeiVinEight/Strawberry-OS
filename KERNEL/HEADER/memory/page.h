#ifndef __KERNEL_PAGE_H__
#define __KERNEL_PAGE_H__

#include <types.h>

void setup_paging();
DWORD linear_mapping(QWORD, QWORD, BYTE);
DWORD identity_mapping(QWORD, BYTE);
QWORD physical_mapping(QWORD);
DWORD AllocatePhysicalMemory(QWORD *, QWORD, QWORD *);
DWORD FreePhysicalMemory(QWORD, QWORD, QWORD);

#endif