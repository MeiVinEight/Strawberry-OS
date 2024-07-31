#include <acpi/fadt.h>
#include <system.h>
#include <console/console.h>
#include <intrinsic.h>
#include <declspec.h>
#include <timer/timer.h>

#define DSDT_SIGNATURE_S5 0x5F35535F

#define PM1_SCI_EN  0x0001
#define PM1_SLP_EN  0x2000

#define ACPI_FADT_STS_POWEROFF  0x0001

CODEDECL ACPI_FADT *FIXED_ACPI_TABLE = 0;
CODEDECL WORD FADT_STATUS = 0;
CODEDECL WORD PM1_SLP_TYP5_A = 0;
CODEDECL WORD PM1_SLP_TYP5_B = 0;

void SetupFADT(ACPI_XSDT *xsdt)
{
	// Find MADT from XSDT
	ACPI_FADT *fadt = 0;
	QWORD size = xsdt->Length - sizeof(ACPI_XSDT);
	QWORD *sdt = (QWORD *) (xsdt + 1);
	while (size)
	{
		ACPI_XSDT *header = (ACPI_XSDT *) (*sdt | SYSTEM_LINEAR);
		if (header->Signature == ACPI_SIGNATURE_FADT)
		{
			fadt = (ACPI_FADT *) header;
			break;
		}
		sdt++;
		size -= 8;
	}
	if (!fadt)
	{
		OUTPUTTEXT("NO FADT\n");
		return;
	}
	OutputXSDT(&fadt->HEAD);
	FIXED_ACPI_TABLE = fadt;

	// Enable SCI
	WORD pm1ac = __inword(fadt->PMAC);
	if (!(pm1ac & PM1_SCI_EN))
	{
		//OUTPUTTEXT("ENABLE SCI\n");
		//while (1) __halt();
		if (!(fadt->SMIC && fadt->AE))
		{
			OUTPUTTEXT("CANNOT ENABLE ACPI\n");
		}
		// Send ACPI_ENABLE cmd
		__outbyte(fadt->SMIC, fadt->AE);

		// wait enable
		QWORD startTime = TimestampCPU();
		while (!(__inword(fadt->PMAC) & PM1_SCI_EN)) __halt();
		QWORD time = TimestampCPU() - startTime;
		if (time > 1000)
		{
			OUTPUTTEXT("ENABLE PM1A TIMEDOUT\n");
			return;
		}
		if (fadt->PMBC)
		{
			QWORD startTime = TimestampCPU();
			while (!(__inword(fadt->PMBC) & PM1_SCI_EN)) __halt();
			time = TimestampCPU() - startTime;
			if (time > 1000)
			{
				OUTPUTTEXT("ENABLE PM2A TIMEDOUT\n");
				return;
			}
		}
	}

	// Find S5 state

	if (!fadt->DSDT || (((ACPI_DSDT *) (fadt->DSDT | SYSTEM_LINEAR))->HEAD.Signature != ACPI_SIGNATURE_DADT))
	{
		OUTPUTTEXT("CANNOT FIND DSDT\n");
		return;
	}

	ACPI_DSDT *dsdt = (ACPI_DSDT *) (fadt->DSDT | SYSTEM_LINEAR);
	OutputXSDT((ACPI_XSDT *) dsdt);
	BYTE *s5Addr = dsdt->DATA;
	DWORD s5Len = dsdt->HEAD.Length - sizeof(ACPI_XSDT);
	while (s5Len && *((DWORD *) s5Addr) != DSDT_SIGNATURE_S5) s5Addr++, s5Len--;
	if (!s5Len)
	{
		S5_NOT_FOUND:;
		OUTPUTTEXT("CANNOT FIND _S5\n");
		return;
	}

	if ((s5Addr[-1] != 0x08) && ((s5Addr[-2] != 0x08) || (s5Addr[-1] != '\\')))
	{
		goto S5_NOT_FOUND;
	}

	s5Addr += 5; // Why ?
	s5Addr += ((*s5Addr & 0xC0) >> 6) + 2;

	if (*s5Addr == 0x0A) s5Addr++;
	PM1_SLP_TYP5_A = *(s5Addr) << 10;
	s5Addr++;
	if (*s5Addr == 0x0A) s5Addr++;
	PM1_SLP_TYP5_B = *(s5Addr) << 10;
	s5Addr++;

	FADT_STATUS |= ACPI_FADT_STS_POWEROFF;
	/*
	else
	{
		OUTPUTTEXT("SCI ENABLED\n");
	}
	*/

	// Try to power off
	// OUTPUTTEXT("TRY TO POWER OFF\n");
	// ACPIPowerOff();
	// while (1) __halt();

	// Power Button
	/*
	DWORD pm1Enable = fadt->PMAE + (fadt->P1EL >> 1);
	__outdword(fadt->PMAE, (1 << 8));
	__outdword(pm1Enable, (1 << 8));
	if (fadt->PMBE)
	{
		pm1Enable = fadt->PMBE + (fadt->P1EL >> 1);
		__outdword(fadt->PMBE, (1 << 8));
		__outdword(pm1Enable, (1 << 8));
	}
	*/
}
void ACPIPowerOff()
{
	if (FADT_STATUS & ACPI_FADT_STS_POWEROFF)
	{
		__outword(FIXED_ACPI_TABLE->PMAC, PM1_SLP_EN | PM1_SLP_TYP5_A);
		if (FIXED_ACPI_TABLE->PMBC)
		{
			__outword(FIXED_ACPI_TABLE->PMBC, PM1_SLP_EN | PM1_SLP_TYP5_B);
		}
	}
}