#include <acpi/madt.h>
#include <system.h>
#include <console/console.h>
#include <interrupt/apic.h>
#include <memory/page.h>
#include <intrinsic.h>
#include <declspec.h>

void SetupMADT(ACPI_XSDT_HEADER *xsdt)
{
	// Find MADT from XSDT
	ACPI_MADT *madt = 0;
	QWORD size = xsdt->Length - sizeof(ACPI_XSDT_HEADER);
	QWORD *sdt = (QWORD *) (xsdt + 1);
	while (size)
	{
		ACPI_XSDT_HEADER *header = (ACPI_XSDT_HEADER *) (*sdt | SYSTEM_LINEAR);
		if (header->Signature == ACPI_SIGNATURE_MADT)
		{
			madt = (ACPI_MADT *) header;
			break;
		}
		sdt++;
		size -= 8;
	}
	if (!madt)
	{
		// No MADT
		return;
	}
	OutputXSDT(&madt->HEADER);
	QWORD buf = 0x20434950414C;
	OUTPUTTEXT((char *) &buf);
	PRINTRAX(madt->LAA, 8);
	LINEFEED();

	DWORD bsp = CurrentAPIC();
	QWORD length = madt->HEADER.Length - sizeof(ACPI_MADT);
	BYTE *data = madt->DATA;
	// Foreach MADT data
	while (length)
	{
		switch (data[0])
		{
			case 0: // Processor Local APIC
			{
				DWORD apicid = data[3];
				if ((*((DWORD *) (data + 4)) & 1) && (apicid != bsp))
				{
					// Startup AP
					StartupAP(apicid);
				}
				break;
			}
		}
		// Next entry
		length -= data[1];
		data += data[1];
	}
}