#include <console/console.h>
#include <declspec.h>
#include <interrupt/interrupt.h>
#include <intrinsic.h>
#include <timer/timer.h>
#include <memory/page.h>
#include <system.h>
#include <memory/segment.h>
#include <memory/heap.h>
#include <memory/virtual.h>
#include <interrupt/apic.h>
#include <acpi/acpi.h>
#include <device/pci.h>

extern BYTE __ImageBase;
CODEDECL const char OSNAME[] = "Strawberry-OS\n";
CODEDECL const char OK[] = "OK\n";
CODEDECL const char MSG0000[] = "SET RSP ";
CODEDECL const char MSG0001[] = "KERNEL AT ";

void _DllMainCRTStartup(OS_SYSTEM_TABLE *table)
{
	setup_segment();
	setup_system_table(table);
	setup_heap();
	setup_screen();

	SCREEN.CLR = 0x0A;
	OUTPUTTEXT(OSNAME);
	SCREEN.CLR = 0x0F;

	OUTPUTTEXT(MSG0000);
	PRINTRAX(__getrsp(), 16);
	LINEFEED();

	OUTPUTTEXT(MSG0001);
	PRINTRAX((QWORD) &__ImageBase, 16);
	LINEFEED();

	OutputCPU();

	SetupCPU();
	setup_interrupt();
	setup_timer();
	setup_paging();
	SetupVirtualMemory();
	SetupACPI();
	SetupPCI();
	
	OUTPUTTEXT(OK);
	while (1) __halt();
}
void APMainStartup()
{
	setup_segment();
	SetupCPU();
	SetupIDT();
	ConfigureAPIC();
	OutputCPU();
	CORE_LOCK = 0;
	while (1) __halt();
}
