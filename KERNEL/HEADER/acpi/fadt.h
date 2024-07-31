#ifndef __KERNEL_FADT_H__
#define __KERNEL_FADT_H__

#include <types.h>
#include <acpi/acpi.h>

#pragma pack(push, 4)
typedef struct _GENERIC_ADDRESS
{
	BYTE AS; // Address space
	BYTE BW; // Bit width
	BYTE BO; // Bit offset
	BYTE SZ; // Access size
	QWORD A; // Address
} GENERIC_ADDRESS;
typedef struct _ACPI_FADT
{
	ACPI_XSDT HEAD;
	DWORD FACS;
	DWORD DSDT;
	// Field used by ACPI 1.0 (INT_MODEL), no longer in use, for compatibility only
	BYTE RSV0;
	BYTE PPMP; // Preferred Power Management Profile
	WORD SCII; // SCI Interrupt
	DWORD SMIC; // SMI Command port
	BYTE AE; // ACPI Enable
	BYTE AD; // ACPI Disable
	BYTE S4BR; // S4BIOS REQ
	BYTE PSC; // PSTATE Control
	DWORD PMAE; // PM1a Event
	DWORD PMBE; // PM1b Event
	DWORD PMAC; // PM1a Control
	DWORD PMBC; // PM1b Control
	DWORD PM2C; // PM2 Control
	DWORD PMTC; // Power Management Timer Control
	DWORD GPE0; // General-Purpose Event 0
	DWORD GPE1; // General-Purpose Event 1
	BYTE P1EL; // PM1a/b Event block Length
	BYTE P1CL; // PM1a/b Control block Length
	BYTE P2CL; // PM2 Control block Length
	BYTE PTCL; // Power Management Timer Control block Length
	BYTE GPE0L;
	BYTE GPE1L;
	BYTE GPE1B; // Offset within the ACPI general-purpose event model where GPE1 based events start.
	BYTE SCTC; // C State Control
	WORD WC2L; // Worst-case hardware latency C2
	WORD WC3L; // Worst-case hardware latency C3
	WORD FSZ; // Flush Size
	WORD FST; // Flush Stride
	BYTE DO; // Duty Offset
	BYTE DW; // Duty Width
	BYTE DA; // Day Alarm
	BYTE MA; // Month Alarm
	BYTE CTR; // Century
	BYTE IBAF[2]; // IA-PC Boot Architecture Flags
	BYTE RSV1;
	DWORD FFF; // Fixed feature flags
	GENERIC_ADDRESS RST;
	BYTE RV; // Reset Value
	BYTE ABAF[2]; // ARM Boot Architecture Flags
	BYTE MV; // FADT Major Version
	QWORD FADX; // Extended physical address of the FACS
	QWORD DSDX; // Extended physical address of the SDST
} ACPI_FADT;
#pragma pack(pop)
typedef struct _ACPI_DSDT
{
	ACPI_XSDT HEAD;
	BYTE      DATA[];
} ACPI_DSDT;

void SetupFADT(ACPI_XSDT *);
void ACPIPowerOff();

#endif