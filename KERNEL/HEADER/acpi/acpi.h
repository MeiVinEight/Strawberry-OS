#ifndef __KERNEL_ACPI_H__
#define __KERNEL_ACPI_H__

#include <types.h>

#define ACPI_SIGNATURE_MADT 0x43495041 // APIC
#define ACPI_SIGNATURE_FADT 0x50434146 // FACP
#define ACPI_SIGNATURE_DADT 0x54445344 // DSDT

typedef struct _ACPI_RSDP
{
	char Signature[8];
	BYTE Checksum;
	char OEMID[6];
	BYTE Revision;
	DWORD RSDT;
	DWORD Length;
	QWORD XSDT;
	BYTE ChecksumX;
	BYTE RSV[3];
} ACPI_RSDP;
typedef struct _ACPI_XSDT
{
	DWORD Signature;
	DWORD Length;
	BYTE Revision;
	BYTE Checksum;
	char OEMID[6];
	char OEMTID[8];
	DWORD OEMR;
	DWORD CID;
	DWORD CR;
} ACPI_XSDT;

void OutputXSDT(ACPI_XSDT *);
int SetupACPI();

#endif