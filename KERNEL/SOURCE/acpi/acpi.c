#include <acpi/acpi.h>
#include <declspec.h>
#include <console/console.h>
#include <system.h>
#include <acpi/madt.h>

CODEDECL const char MSG0A00[] = "SETUP ACPI\n";
CODEDECL const char MSG0A01[] = "FIND RSDP AT ";

void OutputXSDT(ACPI_XSDT_HEADER* t)
{
	char buf[8];
	buf[4] = ' ';
	buf[5] = 0;
	*((DWORD *) buf) = t->Signature;
	OUTPUTTEXT(buf);
	PRINTRAX((QWORD) t, 16);
	LINEFEED();
}
int SetupACPI()
{
	OUTPUTTEXT(MSG0A00);
	ACPI_RSDP *rsdp = (ACPI_RSDP *) (SYSTEM_TABLE.RSDP | SYSTEM_LINEAR);
	if (!rsdp)
	{
		return 1;
	}
	OUTPUTTEXT(MSG0A01);
	PRINTRAX((QWORD) rsdp, 16);
	LINEFEED();

	ACPI_XSDT_HEADER *xsdt = 0;
	if (!rsdp->Revision)
	{
		xsdt = (ACPI_XSDT_HEADER *) (QWORD) (rsdp->RSDT | SYSTEM_LINEAR);
	}
	else if (rsdp->Revision == 2)
	{
		xsdt = (ACPI_XSDT_HEADER *) (rsdp->XSDT | SYSTEM_LINEAR);
	}
	else
	{
		return 1;
	}
	OutputXSDT(xsdt);
	SetupMADT(xsdt);
	return 0;
}