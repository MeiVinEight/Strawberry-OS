#ifndef __KERNEL_MEMORY_VIRTUAL_H__
#define __KERNEL_MEMORY_VIRTUAL_H__

#include <types.h>
#include <memory/block.h>

#define MEM_RESERVE  0x0001
#define MEM_COMMIT   0x0002
#define MEM_UNCOMMIT 0x0004
#define MEM_FREE     0x0008

extern MEMORY_BLOCK *VTL_FRE;
extern MEMORY_BLOCK *VTL_RSV;
extern MEMORY_BLOCK *VTL_CMT;

void SetupVirtualMemory();
void *NtAllocateVirtualMemory(QWORD, QWORD, DWORD, DWORD);

#endif