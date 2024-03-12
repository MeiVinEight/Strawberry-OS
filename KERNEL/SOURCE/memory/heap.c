#include <memory/heap.h>
#include <declspec.h>
#include <memory/page.h>

#define KERNEL_HEAP_PHYSICAL 0x02000000
#define KERNEL_HEAP_LINEAR   0xFFFFFFFFFE000000
#define HEAP_MASK (~(0ULL))

CODEDECL QWORD HEAPK;

void setup_heap()
{
	// Static allocate 0x03000000 + 16M to 0xFFFFFFFFFE000000 as kernel heap
	HEAPK = KERNEL_HEAP_LINEAR;
	QWORD linear = HEAPK;
	for (QWORD addr = KERNEL_HEAP_PHYSICAL; addr < 0x03000000;)
	{
		linear_mapping(addr, linear, 1); // 2M PAGE
		addr += 0x00200000;
		linear += 0x00200000;
	}
	// First 8 byte of each block is payload size
	*((QWORD *) HEAPK) = 0x00FFFFF0;
	// Last block's payload size is ~0
	*((QWORD *) (HEAPK + 0x00FFFFF8)) = HEAP_MASK;
}
void *HeapAlloc(QWORD heap, QWORD size)
{
	// Fix size, 8 byte aligned
	size = (((size - 1) >> 3) + 1) << 3;
	QWORD *block = (QWORD *) heap;
	// Not the last block
	while (~*block)
	{
		// Block is free
		if (!(*block & 1))
		{
			// Merge free blocks
			QWORD *next = block + (*block >> 3);
			next++;
			while (!(*next & 1))
			{
				*block += (*next & (HEAP_MASK << 3));
				*block += 8;
				next += *next >> 3;
				next++;
			}
			// Check block size
			QWORD blockSize = (*block >> 3) << 3;
			if (blockSize >= size)
			{
				// Not whole block
				if (blockSize > size)
				{
					// Split to two blocks
					next = block;
					*next = size;
					next += (size >> 3);
					next++;
					*next = blockSize - size - 8;
				}
				// Use this block
				*block |= 1;
				return ++block;
			}
		}

		// Next block
		block += *block >> 3;
		block++;
	}
	return 0;
}
void HeapFree(QWORD p)
{
	QWORD *block = ((QWORD *) (p - 8));
	// Clear bit 0
	*block = (*block >> 1) << 1;
}