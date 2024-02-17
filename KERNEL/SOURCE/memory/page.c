#include <memory/page.h>
#include <declspec.h>
#include <types.h>
#include <console/console.h>
#include <interrupt/interrupt.h>
#include <intrinsic.h>
#include <system.h>

#define PAGE_COUNT (15 << 8)
CODEDECL const char MSG0500[] = "SETUP PAGING\n";
CODEDECL BYTE PTM[PAGE_COUNT >> 3];
CODEDECL QWORD(*PAGING)[512];

void interrupt_PF(INTERRUPT_STACK* stack)
{
	if (stack->ERROR == 0)
	{
		linear_mapping(__readcr2());
	}
}
void setup_paging()
{
	PAGING = (QWORD(*)[512]) OST->PAGE;
	OUTPUTTEXT(MSG0500);
	PTM[0] = 0x1F;
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
void linear_mapping(QWORD addr)
{
	WORD idx0 = (addr >> 39) & 0x1FF;
	WORD idx1 = (addr >> 30) & 0x1FF;
	WORD idx2 = (addr >> 21) & 0x1FF;
	QWORD *L2 = page_entry(page_entry((QWORD *) __readcr3(), idx0), idx1);
	if (L2)
	{
		L2[idx2] = ((addr >> 21) << 21) | 0x83;
	}
}