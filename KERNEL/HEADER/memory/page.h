#ifndef __KERNEL_PAGE_H__
#define __KERNEL_PAGE_H__

#include <types.h>

// 256PAGE per 1M
#define PAGE_COUNT (15 << 8)

extern BYTE PTM[];
extern QWORD(*PAGING)[512];

void setup_paging();
DWORD linear_mapping(QWORD, QWORD, BYTE);
DWORD identity_mapping(QWORD, BYTE);

#endif