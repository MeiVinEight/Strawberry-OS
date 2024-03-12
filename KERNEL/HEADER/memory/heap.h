#ifndef __KERNEL_HEAP_H__
#define __KERNEL_HEAP_H__

#include <types.h>

extern QWORD HEAPK;

void setup_heap();

void *HeapAlloc(QWORD, QWORD);
void HeapFree(QWORD);

#endif