#include <memory/page.h>
#include <declspec.h>
#include <types.h>
#include <console/console.h>
#include <interrupt/interrupt.h>
#include <intrinsic.h>
#include <system.h>
#include <memory/heap.h>
#include <memory/block.h>
#include <msr.h>
#include <memory/virtual.h>
#include <interrupt/apic.h>

typedef struct _MEMORY_REGION
{
	QWORD A; // Base Address
	QWORD L; // Memory Block Size
	DWORD F; // Flag
	DWORD X; // Extended Flag
} MEMORY_REGION;

CODEDECL const char MSG0500[] = "SETUP PAGING\n";
CODEDECL const char MSG0501[] = "FREE MEMORY ";
CODEDECL const char MSG0502[] = "Base Address       Length             Depth\n";
CODEDECL const char MSG0503[] = "MEMORY NOT ENOUGH ";
CODEDECL const char MSG0504[] = "ENABLE IA32_EFER.NXE\n";
CODEDECL MEMORY_BLOCK *PHYSICAL_MEMORY_MAP;

void MemoryNotEnough(DWORD code)
{
	SCREEN.CLR = 0x0C;
	OUTPUTTEXT(MSG0503);
	PRINTRAX(code, 2);
	SCREEN.CLR = 0x0F;
	LINEFEED();
	while (1) __halt();
}
void INT0E(INTERRUPT_STACK* stack)
{
	QWORD CR2 = __readcr2();
	QWORD apicid = CurrentAPIC();
	if (!(stack->ERROR & 1))
	{
		if (CR2 >= 0xFFFF800000000000ULL)
		{
			if (CR2 < 0xFFFF900000000000ULL)
			{
				// 0xFFFF800000000000+0x0000100000000000 Statically Mapping
				// In this area, LinearAddress = PhysicalAddress | 0xFFFF800000000000ULL
				// Use 1G page
				linear_mapping(((CR2 << 17) >> 17), CR2, PAGE4_1G, 7);
				return;
			}
			else if (CR2 < 0xFFFFE00000000000ULL)
			{
				// 0xFFFF900000000000+0x0000100000000000 Dynamic Allocate Space
				MEMORY_BLOCK *cr = SearchMemoryNode(&VTL_CMT, CR2, 0);
				if (cr)
				{
					QWORD physicalAddress = 0;
					QWORD pageCount = 1;
					QWORD retn = AllocatePhysicalMemory(&physicalAddress, 0, &pageCount);
					if (!pageCount)
					{
						MemoryNotEnough(retn);
					}
					linear_mapping(physicalAddress, CR2, PAGE4_4K, (cr->V << 1) | 1);
					return;
				}
			}
			else if (CR2 < 0xFFFFF00000000000ULL)
			{
				QWORD idx3 = (CR2 >> 12) & 0x1FF;
				if (idx3)
				{
					QWORD physicalAddress = 0;
					QWORD pageCount = 1;
					AllocatePhysicalMemory(&physicalAddress, PAGE4_4K, &pageCount);
					linear_mapping(physicalAddress, CR2, PAGE4_4K, 7);
					return;
				}
				// Stack Overflow
			}
		}
	}
	QWORD prefix = 0x2320555043;
	OUTPUTTEXT((char *) &prefix);
	PRINTRAX(apicid, 2);
	OUTCHAR(' ');
	SCREEN.CLR = 0x0C;
	char buf[5] = { '#', 'P', 'F', 0, 0 };
	OUTPUTTEXT(buf);
	SCREEN.CLR = 0x0F;
	LINEFEED();
	buf[0] = 'C';
	buf[1] = 'R';
	buf[2] = '2';
	buf[3] = ' ';
	OUTPUTTEXT(buf);
	PRINTRAX(CR2, 16);
	LINEFEED();
	buf[0] = 'E';
	buf[1] = 'R';
	buf[2] = 'R';
	OUTPUTTEXT(buf);
	PRINTRAX(stack->ERROR, 16);
	LINEFEED();
	buf[0] = 'R';
	buf[1] = 'I';
	buf[2] = 'P';
	buf[3] = ' ';
	OUTPUTTEXT(buf);
	PRINTRAX(stack->RIP, 16);
	LINEFEED();
	while (1) __halt();
}
QWORD empty_page()
{
	QWORD page = 0;
	QWORD pcnt = 1;
	DWORD retn = AllocatePhysicalMemory(&page, 0, &pcnt);
	if (pcnt)
	{
		return page;
	}
	MemoryNotEnough(retn);
	return 0;
}
QWORD *page_entry(QWORD *PT, WORD idx)
{
	if (PT && (idx < 512))
	{
		if (!(PT[idx] & 1))
		{
			QWORD l = empty_page();
			memset((QWORD *) (l | SYSTEM_LINEAR), 0, 4096);
			PT[idx] = l | 3;
		}
		if (PT[idx] & 0x80) return 0;
		return (QWORD *) ((PT[idx] & ~0xFFF) | SYSTEM_LINEAR);
	}
	return 0;
}
DWORD linear_mapping(QWORD addr, QWORD linear, BYTE size, QWORD options)
{
	// BIT
	// 63: No Execute
	// 1: Has write
	// 0: Present
	options ^= 4;
	options = ((options & 4) << 61) | (options & 3);
	// Resolve linear address
	WORD idx0 = (linear >> 39) & 0x1FF;
	WORD idx1 = (linear >> 30) & 0x1FF;
	WORD idx2 = (linear >> 21) & 0x1FF;
	WORD idx3 = (linear >> 12) & 0x1FF;

	QWORD *L0 = (QWORD *) (__readcr3() | SYSTEM_LINEAR);
	QWORD *L1 = page_entry(L0, idx0);
	if (L1 && size == PAGE4_1G && !(L1[idx1] & 1))
	{
		// 1G PAGING
		L1[idx1] = ((addr >> 30) << 30) | 0x80;
		L1[idx1] |= options;
		return 0;
	}
	QWORD *L2 = page_entry(L1, idx1);
	if (L2 && size == PAGE4_2M && !(L2[idx2] & 1))
	{
		// 2M PAGING
		L2[idx2] = ((addr >> 21) << 21) | 0x80;
		L2[idx2] |= options;
		return 0;
	}
	QWORD *L3 = page_entry(L2, idx2);
	if (L3 && size == PAGE4_4K && !(L3[idx3] & 1))
	{
		// 4K PAGING
		L3[idx3] = ((addr >> 12) << 12);
		L3[idx3] |= options;
		return 0;
	}
	// Mapping failed
	return 1;
}
DWORD linear_unmapping(QWORD linear)
{
	// Resolve linear address
	WORD idx0 = (linear >> 39) & 0x1FF;
	WORD idx1 = (linear >> 30) & 0x1FF;
	WORD idx2 = (linear >> 21) & 0x1FF;
	WORD idx3 = (linear >> 12) & 0x1FF;

	QWORD *L0 = (QWORD *) (__readcr3() | SYSTEM_LINEAR);
	if (!(L0[idx0] & 1))
	{
		return 0xFF;
	}

	QWORD *L1 = (QWORD *) ((L0[idx0] & ~0xFFFULL) | SYSTEM_LINEAR);
	if (!(L1[idx1] & 1))
	{
		return 0xFF;
	}
	if (L1[idx1] & 0x80)
	{
		L1[idx1] ^= 1;
		return PAGE4_1G;
	}

	QWORD *L2 = (QWORD *) ((L1[idx1] & ~0xFFFULL) | SYSTEM_LINEAR);
	if (!(L2[idx2] & 1))
	{
		return 0xFF;
	}
	if (L2[idx2] & 0x80)
	{
		L2[idx2] ^= 1;
		return PAGE4_2M;
	}

	QWORD *L3 = (QWORD *) ((L2[idx2] & !0xFFFULL) | SYSTEM_LINEAR);
	if (!(L3[idx3] & 1))
	{
		return 0xFF;
	}
	L3[idx3] ^= 1;
	return PAGE4_4K;
}
QWORD physical_mapping(QWORD linear)
{
	QWORD addressMask = 0x07FFFFFFFFFFF000;
	WORD idx0 = (linear >> 39) & 0x1FF;
	WORD idx1 = (linear >> 30) & 0x1FF;
	WORD idx2 = (linear >> 21) & 0x1FF;
	WORD idx3 = (linear >> 12) & 0x1FF;

	QWORD *L0 = (QWORD *) (__readcr3() | SYSTEM_LINEAR);
	if (!(L0[idx0] & 1))
	{
		return ~(0ULL);
	}

	QWORD *L1 = (QWORD *) ((L0[idx0] & ~0xFFF) | SYSTEM_LINEAR);
	if (!(L1[idx1] & 1))
	{
		return ~(0ULL);
	}
	if (L1[idx1] & 0x80)
	{
		return (L1[idx1] & addressMask) + (linear & ((1 << 30) - 1));
	}

	QWORD *L2 = (QWORD *) ((L1[idx1] & ~0xFFF) | SYSTEM_LINEAR);
	if (!(L2[idx2] & 1))
	{
		return ~(0ULL);
	}
	if (L2[idx2] & 0x80)
	{
		return (L2[idx2] & addressMask) + (linear & ((1 << 21) - 1));
	}

	QWORD *L3 = (QWORD *) ((L2[idx2] & ~0xFFF) | SYSTEM_LINEAR);
	if (!(L3[idx3] & 1))
	{
		return ~(0ULL);
	}
	return (L3[idx3] & addressMask) + (linear & ((1 << 12) - 1));
}
QWORD ForeachMemoryMap(MEMORY_BLOCK *block, DWORD height, DWORD flag)
{
	QWORD free = 0;
	if (block)
	{
		DWORD split = 0x00207C20; // " | "
		free += ForeachMemoryMap(block->L, height + 1, flag);
		PRINTRAX(block->A, 16);
		OUTPUTTEXT((char *) &split);
		PRINTRAX(block->S, 16);
		if (flag & 1)
		{
			OUTPUTTEXT((char *) &split);
			PRINTRAX(block->V, 1);
		}
		OUTPUTTEXT((char *) &split);
		PRINTRAX(height, 2);
		LINEFEED();
		free += block->S;
		free += ForeachMemoryMap(block->R, height + 1, flag);
	}
	return free;
}
void setup_paging()
{
	OUTPUTTEXT(MSG0500);
	OUTPUTTEXT(MSG0504);
	__writemsr(IA32_EFER_MSR, __readmsr(IA32_EFER_MSR) | (1 << 11));

	MEMORY_REGION *beg = (MEMORY_REGION *) (SYSTEM_TABLE.MMAP + 8);
	MEMORY_REGION *end = (MEMORY_REGION *) (*((QWORD *) SYSTEM_TABLE.MMAP) | SYSTEM_LINEAR);
	QWORD total = 0;
	while (beg < end)
	{
		if (beg->F == 1 && beg->L)
		{
			MEMORY_BLOCK *block = (MEMORY_BLOCK *) HeapAlloc(HEAPK, sizeof(MEMORY_BLOCK));
			memset(block, 0, sizeof(MEMORY_BLOCK));
			block->A = beg->A;
			block->S = beg->L;
			InsertMemoryNode(&PHYSICAL_MEMORY_MAP, block, 1);
		}
		total += beg->L;
		beg++;
	}
	OUTPUTTEXT(MSG0502);
	QWORD free = ForeachMemoryMap(PHYSICAL_MEMORY_MAP, 1, 0);
	OUTPUTTEXT(MSG0501);
	PRINTRAX(free, 16);
	OUTCHAR(' ');
	OUTCHAR('/');
	OUTCHAR(' ');
	PRINTRAX(total, 16);
	LINEFEED();
	register_interrupt(0x0E, INT0E);
}
DWORD AllocatePhysicalMemory(QWORD *physicalAddress, QWORD pageSize, QWORD *pageCount)
{
	if (!*pageCount)
	{
		return 0;
	}

	// Page size: 0=4K 1=2M 2=1G
	if (pageSize > 2)
	{
		*pageCount = 0;
		return 1;
	}

	// Page size = 1 << shift
	QWORD pageShift = 12 + (pageSize * 9);

	// First, tmp.A = 0
	QWORD searchAddress = 0;
	while (1)
	{
		MEMORY_BLOCK *min = SearchMemoryNode(&PHYSICAL_MEMORY_MAP, searchAddress, 1);
		// min == 0 means no more memory block is usable
		if (!min)
		{
			*pageCount = 0;
			MemoryNotEnough(2);
			return 2;
		}
		searchAddress = min->A + min->S;

		QWORD blockSize = min->S;
		QWORD blockAddr = min->A;
		// Align block address to page size
		QWORD reminder = blockAddr & ((1ULL << pageShift) - 1);
		reminder = (-reminder) & ((1ULL << pageShift) - 1);
		// Cannot align this block to page size
		// Continue to next block
		if (blockSize < reminder)
		{
			continue;
		}
		blockSize -= reminder;
		blockAddr += reminder;
		// Cannot allocate one page from this block
		if (blockSize < (1ULL << pageShift))
		{
			continue;
		}
		// Allocate as more page as possible
		*physicalAddress = blockAddr;
		QWORD allocateCount = blockSize >> pageShift;
		if (allocateCount < *pageCount)
		{
			*pageCount = allocateCount;
		}
		allocateCount = *pageCount;
		// Reduce this block
		min->S = blockAddr - min->A;
		if (!min->S)
		{
			RemoveMemoryNode(&PHYSICAL_MEMORY_MAP, min);
		}
		// This allocation split the block to two block
		if (blockSize - (allocateCount << pageShift))
		{
			blockAddr += (allocateCount << pageShift);
			blockSize -= (allocateCount << pageShift);
			MEMORY_BLOCK *block = (MEMORY_BLOCK *) HeapAlloc(HEAPK, sizeof(MEMORY_BLOCK));
			memset(block, 0, sizeof(MEMORY_BLOCK));
			block->A = blockAddr;
			block->S = blockSize;
			InsertMemoryNode(&PHYSICAL_MEMORY_MAP, block, 1);
		}
		return 0;
	}
}
DWORD FreePhysicalMemory(QWORD physicalAddress, QWORD pageSize, QWORD pageCount)
{
	if (pageSize > 2)
	{
		return 1;
	}
	if (!pageCount)
	{
		return 0;
	}
	if (!(physicalAddress + 1)) return 0; // SKIP NULL ADDRESS 0xFFFFFFFFFFFFFFFF (~0ULL)

	QWORD page = 1ULL << (12 + 9 * pageSize);
	MEMORY_BLOCK *block = (MEMORY_BLOCK *) HeapAlloc(HEAPK, sizeof(MEMORY_BLOCK));
	memset(block, 0, sizeof(MEMORY_BLOCK));
	block->A = physicalAddress;
	block->S = page * pageCount;
	InsertMemoryNode(&PHYSICAL_MEMORY_MAP, block, 1);
	return 0;
}
