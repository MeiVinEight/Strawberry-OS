#include <memory/page.h>
#include <declspec.h>
#include <types.h>
#include <console/console.h>
#include <interrupt/interrupt.h>
#include <intrinsic.h>
#include <system.h>

CODEDECL const char MSG0500[] = "SETUP PAGING\n";
CODEDECL BYTE PTM[PAGE_COUNT >> 3];
CODEDECL QWORD(*PAGING)[512];

void interrupt_PF(INTERRUPT_STACK* stack)
{
	if (stack->ERROR == 0)
	{
		if (identity_mapping(__readcr2(), 0))
		{
			char buf[4] = {'#', 'P', 'F', 0};
			OUTPUTTEXT(buf);
			while (1) __halt();
		}
	}
}
void setup_paging()
{
	OUTPUTTEXT(MSG0500);
	register_interrupt(0x0E, interrupt_PF);
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
			PT[idx] = l | 3;
		}
		return (QWORD *) (PT[idx] & ~0xFFF);
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

	QWORD *L0 = (QWORD *) __readcr3();
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