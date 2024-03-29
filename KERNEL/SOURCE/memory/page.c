#include <memory/page.h>
#include <declspec.h>
#include <types.h>
#include <console/console.h>
#include <interrupt/interrupt.h>
#include <intrinsic.h>
#include <system.h>
#include <memory/heap.h>
#include <memory/block.h>

#define SYSTEM_LINEAR 0xFFFF800000000000ULL

typedef struct _MEMORY_REGION
{
	QWORD A;
	QWORD L;
	DWORD F;
	DWORD X;
} MEMORY_REGION;

CODEDECL const char MSG0500[] = "SETUP PAGING\n";
CODEDECL const char MSG0501[] = "FREE MEMORY ";
CODEDECL const char MSG0502[] = "Base Address       Length             Height\n";
CODEDECL BYTE PTM[PAGE_COUNT >> 3];
CODEDECL QWORD(*PAGING)[512];
CODEDECL MEMORY_BLOCK *MEMORY_MAP;

void interrupt_PF(INTERRUPT_STACK* stack)
{
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
	PRINTRAX(__readcr2(), 16);
	LINEFEED();
	buf[0] = 'R';
	buf[1] = 'I';
	buf[2] = 'P';
	buf[3] = ' ';
	OUTPUTTEXT(buf);
	PRINTRAX(stack->RIP, 16);
	LINEFEED();
	while (1) __halt();
	/*
	if (stack->ERROR == 0)
	{
		if (identity_mapping(__readcr2(), 0))
		{
			char buf[4] = {'#', 'P', 'F', 0};
			OUTPUTTEXT(buf);
			while (1) __halt();
		}
	}
	*/
}
QWORD empty_page()
{
	for (DWORD i = 0; i < PAGE_COUNT; i++)
	{
		if (!(PTM[i >> 3] & (1 << (i & 7))))
		{
			PTM[i >> 3] |= (1 << (i & 7));
			return (QWORD) PAGING[i];
		}
	}
	return 0;
}
QWORD *page_entry(QWORD *PT, WORD idx)
{
	if (PT && (idx < 512))
	{
		if (!(PT[idx] & 1))
		{
			QWORD l = empty_page();
			if (!l)
			{
				IDT[0x0E].P = 0;
				return 0;
			}
			memset((QWORD *) l, 0, 4096);
			PT[idx] = physical_mapping(l) | 3;
		}
		if (PT[idx] & 0x80) return 0;
		return (QWORD *) ((PT[idx] & ~0xFFF) | SYSTEM_LINEAR);
	}
	return 0;
}
DWORD linear_mapping(QWORD addr, QWORD linear, BYTE size)
{
	// Resolve linear address
	WORD idx0 = (linear >> 39) & 0x1FF;
	WORD idx1 = (linear >> 30) & 0x1FF;
	WORD idx2 = (linear >> 21) & 0x1FF;
	WORD idx3 = (linear >> 12) & 0x1FF;

	QWORD *L0 = (QWORD *) (__readcr3() | SYSTEM_LINEAR);
	QWORD *L1 = page_entry(L0, idx0);
	if (L1 && size == 2 && !(L1[idx1] & 1))
	{
		// 1G PAGING
		L1[idx1] = ((addr >> 30) << 30) | 0x83;
		return 0;
	}
	QWORD *L2 = page_entry(L1, idx1);
	if (L2 && size == 1 && !(L2[idx2] & 1))
	{
		// 2M PAGING
		L2[idx2] = ((addr >> 21) << 21) | 0x83;
		return 0;
	}
	QWORD *L3 = page_entry(L2, idx2);
	if (L3 && size == 0 && !(L3[idx3] & 1))
	{
		// 4K PAGING
		L3[idx3] = ((addr >> 12) << 12) | 0x03;
		return 0;
	}
	// Mapping failed
	return 1;
}
DWORD identity_mapping(QWORD physical, BYTE size)
{
	return linear_mapping(physical, physical, size);
}
QWORD physical_mapping(QWORD linear)
{
	WORD idx0 = (linear >> 39) & 0x1FF;
	WORD idx1 = (linear >> 30) & 0x1FF;
	WORD idx2 = (linear >> 21) & 0x1FF;
	WORD idx3 = (linear >> 12) & 0x1FF;

	QWORD *L0 = (QWORD *) (__readcr3() | SYSTEM_LINEAR);
	if (!(L0[idx0] | 1))
	{
		return ~(0ULL);
	}

	QWORD *L1 = (QWORD *) ((L0[idx0] & ~0xFFF) | SYSTEM_LINEAR);
	if (!(L1[idx1] | 1))
	{
		return ~(0ULL);
	}
	if (L1[idx1] & 0x80)
	{
		return (L1[idx1] & ~0xFFF) + (linear & ((1 << 30) - 1));
	}

	QWORD *L2 = (QWORD *) ((L1[idx1] & ~0xFFF) | SYSTEM_LINEAR);
	if (!(L2[idx2] | 1))
	{
		return ~(0ULL);
	}
	if (L2[idx2] & 0x80)
	{
		return (L2[idx2] & ~0xFFF) + (linear & ((1 << 21) - 1));
	}

	QWORD *L3 = (QWORD *) ((L2[idx2] & ~0xFFF) | SYSTEM_LINEAR);
	if (!(L3[idx3] & 1))
	{
		return ~(0ULL);
	}
	return (L3[idx3] & ~0xFFF) + (linear & ((1 << 12) - 1));
}
void ForeachMemoryMap(MEMORY_BLOCK *block, int height)
{
	if (block)
	{
		DWORD split = 0x00207C20; // " | "
		ForeachMemoryMap(block->L, height + 1);
		PRINTRAX(block->A, 16);
		OUTPUTTEXT((char *) &split);
		PRINTRAX(block->S, 16);
		OUTPUTTEXT((char *) &split);
		PRINTRAX(height, 2);
		LINEFEED();
		ForeachMemoryMap(block->R, height + 1);
	}
}
void setup_paging()
{
	OUTPUTTEXT(MSG0500);
	MEMORY_REGION *beg = (MEMORY_REGION *) (OST.MMAP + 8);
	MEMORY_REGION *end = (MEMORY_REGION *) (*((QWORD *) OST.MMAP) | SYSTEM_LINEAR);
	QWORD usable = 0;
	QWORD total = 0;
	while (beg < end)
	{
		if (beg->F == 1)
		{
			if (beg->A >= 0x03000000ULL)
			{
				MEMORY_BLOCK *block = (MEMORY_BLOCK *) HeapAlloc(HEAPK, sizeof(MEMORY_BLOCK));
				memset(block, 0, sizeof(MEMORY_BLOCK));
				block->A = beg->A;
				block->S = beg->L;
				usable += block->S;
				InsertMemoryNode(&MEMORY_MAP, block);
			}
			else if (beg->L >= (0x03000000ULL - beg->A))
			{
				MEMORY_BLOCK *block = (MEMORY_BLOCK *) HeapAlloc(HEAPK, sizeof(MEMORY_BLOCK));
				memset(block, 0, sizeof(MEMORY_BLOCK));
				block->A = 0x03000000;
				block->S = beg->L - (0x03000000ULL - beg->A);
				usable += block->S;
				InsertMemoryNode(&MEMORY_MAP, block);
			}
		}
		total += beg->L;
		beg++;
	}
	OUTPUTTEXT(MSG0502);
	//          0000000000000000 | 0000000000000000 | 00
	ForeachMemoryMap(MEMORY_MAP, 1);
	OUTPUTTEXT(MSG0501);
	PRINTRAX(usable, 16);
	OUTCHAR(' ');
	OUTCHAR('/');
	OUTCHAR(' ');
	PRINTRAX(total, 16);
	LINEFEED();
	register_interrupt(0x0E, interrupt_PF);
}