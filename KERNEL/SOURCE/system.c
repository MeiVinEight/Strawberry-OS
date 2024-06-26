#include <system.h>
#include <declspec.h>
#include <intrinsic.h>
#include <console/console.h>
#include <memory/page.h>
#include <console/console.h>
#include <interrupt/apic.h>

CODEDECL OS_SYSTEM_TABLE SYSTEM_TABLE;
CODEDECL const char MSG0700[] = "SETUP STREAMING SIMD EXTENSION\n";
CODEDECL const char MSG0701[] = "SSE 4.2\n";
CODEDECL const char MSG0702[] = "SSE 4.1\n";
CODEDECL const char MSG0703[] = "SSSE 3\n";
CODEDECL const char MSG0704[] = "SSE 3\n";
CODEDECL const char MSG0705[] = "SSE 2\n";
CODEDECL const char MSG0706[] = "SSE\n";
CODEDECL const char MSG0707[] = "NO SSE\n";
CODEDECL BYTE CORE_LOCK = 0;

void setup_system_table(OS_SYSTEM_TABLE *table)
{
	memcpy(&SYSTEM_TABLE, table, sizeof(OS_SYSTEM_TABLE));
	memcpy(&SCREEN, (void *) SYSTEM_TABLE.SCRN, sizeof(CONSOLE_SCREEN));
}
void SetupCPU()
{
	OUTPUTTEXT(MSG0700);
	DWORD cpuid[4] = {0};

	// SSE
	__cpuid(cpuid, 1);
	if (cpuid[2] & (1 << 20))
	{
		OUTPUTTEXT(MSG0701);
	}
	else if (cpuid[2] & (1 << 19))
	{
		OUTPUTTEXT(MSG0702);
	}
	else if (cpuid[2] & (1 << 9))
	{
		OUTPUTTEXT(MSG0703);
	}
	else if (cpuid[2] & 1)
	{
		OUTPUTTEXT(MSG0704);
	}
	else if (cpuid[3] & (1 << 26))
	{
		OUTPUTTEXT(MSG0705);
	}
	else if (cpuid[3] & (1 << 25))
	{
		OUTPUTTEXT(MSG0706);
	}
	else
	{
		OUTPUTTEXT(MSG0707);
	}
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