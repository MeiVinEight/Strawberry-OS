#include <console/console.h>
#include <declspec.h>
#include <interrupt/interrupt.h>
#include <intrinsic.h>
#include <timer/timer.h>
#include <memory/page.h>

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

	OUTPUTTEXT(OK);
	while (1) __halt();
}