#include <system.h>
#include <declspec.h>
#include <intrinsic.h>
#include <console/console.h>
#include <memory/page.h>
#include <console/console.h>
#include <interrupt/apic.h>

CODEDECL OS_SYSTEM_TABLE SYSTEM_TABLE;
CODEDECL BYTE CORE_LOCK = 0;

void setup_system_table(OS_SYSTEM_TABLE *table)
{
	memcpy(&SYSTEM_TABLE, table, sizeof(OS_SYSTEM_TABLE));
	memcpy(&SCREEN, (void *) SYSTEM_TABLE.SCRN, sizeof(CONSOLE_SCREEN));
}
void SetupCPU()
{
	// SSE
	DWORD cpuid[4] = { 0 };
	__cpuid(cpuid, 1);
	QWORD sse = 0;
	sse |= cpuid[2] & (1 << 20); // SSE 4.2
	sse |= cpuid[2] & (1 << 19); // SSE 4.1
	sse |= cpuid[2] & (1 << 9);  // SSSE 3
	sse |= cpuid[2] & 1;         // SSE 3
	sse |= cpuid[3] & (1 << 26); // SSE 2
	sse |= cpuid[3] & (1 << 26); // SSE
	if (sse)
	{
		QWORD CR0 = __readcr0();
		// CR0.EM = 0
		CR0 &= ~4;
		// CR0.MP = 1
		CR0 |= 2;
		__writecr0(CR0);
		QWORD CR4 = __readcr4();
		// CR4.OSFXSR = 1
		// CR4.OSXMMEXCPT = 1
		CR4 |= 0x600;
		__writecr4(CR4);
	}
}
void OutputCPU()
{
	char brand[50];
	brand[0] = 'C';
	brand[1] = 'P';
	brand[2] = 'U';
	brand[3] = ' ';
	brand[4] = '#';
	brand[5] = 0;
	OUTPUTTEXT(brand);
	PRINTRAX(CurrentAPIC(), 2);
	OUTCHAR(' ');
	memset(brand, 0, 50);
	__cpuid((int *) (brand), 0x80000002);
	__cpuid((int *) (brand + 16), 0x80000003);
	__cpuid((int *) (brand + 32), 0x80000004);
	OUTPUTTEXT(brand);
	LINEFEED();
}