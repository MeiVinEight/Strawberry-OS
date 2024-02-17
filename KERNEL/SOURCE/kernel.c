#include <console/console.h>
#include <declspec.h>
#include <interrupt/interrupt.h>
#include <intrinsic.h>
#include <timer/timer.h>
#include <memory/page.h>

typedef struct _MEMORY_REGION
{
	QWORD A;
	QWORD L;
	DWORD F;
	DWORD X;
} MEMORY_REGION;

extern BYTE __ImageBase;
CODEDECL const char OSNAME[] = "Strawberry-OS\n";
CODEDECL const char OK[] = "OK\n";
CODEDECL const char MSG0000[] = "SET RSP ";
CODEDECL const char MSG0001[] = "KERNEL AT ";

void _DllMainCRTStartup(void)
{
	screen.color = 0x0A;
	OUTPUTTEXT(OSNAME);
	screen.color = 0x0F;

	OUTPUTTEXT(MSG0000);
	PRINTRAX(__getrsp(), 16);
	LINEFEED();

	OUTPUTTEXT(MSG0001);
	PRINTRAX((QWORD) &__ImageBase, 16);
	LINEFEED();

	char brand[50];
	brand[0] = 'C';
	brand[1] = 'P';
	brand[2] = 'U';
	brand[3] = ':';
	brand[4] = 0;
	OUTPUTTEXT(brand);
	memset(brand, 0, 50);
	__cpuid((int*)(brand), 0x80000002);
	__cpuid((int*)(brand + 16), 0x80000003);
	__cpuid((int*)(brand + 32), 0x80000004);
	OUTPUTTEXT(brand);
	LINEFEED();

	setup_interrupt();
	setup_timer();
	setup_paging();
	MEMORY_REGION *beg = (MEMORY_REGION *) 0x00000608;
	MEMORY_REGION *end = *((MEMORY_REGION **) 0x00000600);
	OUTPUTTEXT("Base Address       Length             Type\n");
	//          0000000000000000 | 0000000000000000 | 00000000
	while (beg < end)
	{
		PRINTRAX(beg->A, 16);
		OUTPUTTEXT(" | ");
		PRINTRAX(beg->L, 16);
		OUTPUTTEXT(" | ");
		PRINTRAX(beg->F, 8);
		LINEFEED();
		beg++;
	}

	OUTPUTTEXT(OK);
	while (1) __halt();
}