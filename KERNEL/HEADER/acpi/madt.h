#ifndef __KERNEL_ACPI_MADT_H__
#define __KERNEL_ACPI_MADT_H__

#include <acpi/acpi.h>

typedef struct _ACPI_MADT
{
	ACPI_XSDT HEADER; // General Header
	DWORD LAA;               // Local APIC Address
	DWORD F0;                // Flags
	BYTE DATA[];             // Rrecord list
} ACPI_MADT;
typedef struct _ACPI_MADT_PLAPIC // Processor Local APIC
{
	BYTE TYPE; // Always 0
	BYTE SIZE; // Always 8
	BYTE PID;  // APIC Processor ID
	BYTE AID;  // APIC ID
	DWORD F1;  // Flags
} ACPI_MADT_PLAPIC;

void SetupMADT(ACPI_XSDT *);

#endif