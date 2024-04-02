#ifndef __KERNEL_MEMORY_BLOCK_H__
#define __KERNEL_MEMORY_BLOCK_H__

#include <types.h>

typedef struct _MEMORY_BLOCK
{
	struct _MEMORY_BLOCK *P;
	struct _MEMORY_BLOCK *L;
	struct _MEMORY_BLOCK *R;
	QWORD H;
	QWORD A;
	QWORD S;
	QWORD V;
} MEMORY_BLOCK;

MEMORY_BLOCK **NodeReference(MEMORY_BLOCK **, MEMORY_BLOCK *);
QWORD TreeHeight(MEMORY_BLOCK *);
void AdjustMemoryMap(MEMORY_BLOCK **, MEMORY_BLOCK *);
void RemoveMemoryNode(MEMORY_BLOCK **, MEMORY_BLOCK *);
void InsertMemoryNode(MEMORY_BLOCK **, MEMORY_BLOCK *, DWORD);
MEMORY_BLOCK *SearchMemoryNode(MEMORY_BLOCK **, QWORD, DWORD);

#endif