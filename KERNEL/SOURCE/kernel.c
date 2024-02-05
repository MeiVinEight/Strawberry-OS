#include <console/console.h>
#include <declspec.h>
#include <interrupt/interrupt.h>
#include <intrinsic.h>
#include <timer/timer.h>

CODEDECL const char OSNAME[] = "Strawberry-OS\n";
CODEDECL const char OK[] = "OK\n";
CODEDECL const char MSG0000[] = "SET RSP=";

void _DllMainCRTStartup()
{
	__setrsp(0x00080000);
	screen.color = 0x0A;
	OUTPUTTEXT(OSNAME);
	screen.color = 0x0F;

	OUTPUTTEXT(MSG0000);
	PRINTRAX(__getrsp(), 16);
	LINEFEED();

	setup_interrupt();
	setup_timer();

	OUTPUTTEXT(OK);
	while (1) __halt();
}