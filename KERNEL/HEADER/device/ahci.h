#ifndef __KERNEL_DEVICE_AHCI_H__
#define __KERNEL_DEVICE_AHCI_H__

#include <device/pci.h>
#include <device/disk.h>

typedef struct _AHCI_HBA_PORT
{
	QWORD CLB; // Command List Base Address
	QWORD FIS; // FIS Base Address
	DWORD IST; // Interrupt Status
	DWORD IEN; // Interrupt Enable
	DWORD CMD; // Command and Status
	DWORD RS0;
	DWORD TFD; // Task File Data
	DWORD SIG; // Signature
	DWORD SAS; // Serial ATA Status
	DWORD SAC; // Serial ATA Control
	DWORD SAE; // Serial ATA Error
	DWORD SAA; // Serial ATA Active
	DWORD CIS; // Command Issue
	DWORD SAN; // Serial ATA Notification
	DWORD FBS; // FIS Based Switching Control
	DWORD SLP; // Device Sleep
	DWORD RS1[0x0A];
	DWORD VEN[0x04];
} AHCI_HBA_PORT;
typedef struct _AHCI_HBA_MEMORY
{
	DWORD         CAP; // Capabilities
	DWORD         GHC; // Global Host Control
	DWORD         INT; // Interrupt Status
	DWORD         PTI; // Ports Implemented
	DWORD         VER; // Version
	DWORD         CCC; // Command Completion Coalescing Control
	DWORD         CCP; // Command Completion Coalescing Ports
	DWORD         EML; // Enclosure Management Location
	DWORD         EMC; // Enclosure Management Control
	DWORD         CAX; // Host Capabilities Extended
	DWORD         HCS; // BIOS/OS Handoff Control and Status
	BYTE          RSV[0x0074];
	BYTE          VSR[0x0060]; // Vendor Specific Registers
	AHCI_HBA_PORT PRT[];
} AHCI_HBA_MEMORY;
typedef struct _AHCI_COMMAND_HEAD
{
	DWORD CFL:0x05; // Command FIS Length
	DWORD API:0x01; // ATAPI
	DWORD WRT:0x01; // WRITE(1), READ(0)
	DWORD PFT:0x01; // Prefetchable
	DWORD RST:0x01; // Reset
	DWORD TST:0x01; // Built-In Self-Test(BIST)
	DWORD BSY:0x01; // Clear Busy upon R_OK
	DWORD RV0:0x01;
	DWORD PMP:0x04; // Port Multiplier Port
	DWORD DTL:0x10; // Physical Region Descriptor Table Length

	DWORD DBC     ; // Physical Region Descriptor Byte Count

	QWORD TBL     ; // Command Table Descriptor Base Address

	DWORD RV1[0x04];
} AHCI_COMMAND_HEAD;
typedef struct _AHCI_PRDT_ENTRY
{
	QWORD DBA     ; // Data Base Address
	DWORD RV0     ;
	DWORD DBC:0x16; // PRD Byte Count
	DWORD RV1:0x09;
	DWORD IOC:0x01; // Interrupt On Completion
} AHCI_PRDT_ENTRY;
typedef struct _AHCI_FIS_H2D
{
	BYTE  TYP     ; // FIS_TYPE_REG_H2D
	BYTE  PMP:0x04; // Port Multiplier Port
	BYTE  RV0:0x03;
	BYTE  CCC:0x01; // CMD(1), CTRL(0)
	BYTE  CMD     ; // Command Register
	BYTE  FTL     ; // Feature Register L

	BYTE  BA0     ; // LBA 0
	BYTE  BA1     ; // LBA 1
	BYTE  BA2     ; // LBA 2
	BYTE  DVC     ; // Device

	BYTE  BA3     ; // LBA 3
	BYTE  BA4     ; // LBA 4
	BYTE  BA5     ; // LBA 5
	BYTE  FTH     ; // Feature Register H

	WORD  CNT     ; // Count
	BYTE  ICC     ; // Isochronous Command Completion
	BYTE  CTR     ; // Control

	BYTE  RV1[0x30];
} AHCI_FIS_H2D;
typedef struct _AHCI_COMMAND_TABLE
{
	AHCI_FIS_H2D    FIS;
	BYTE            ATA[0x10];
	BYTE            RSV[0x30];
	AHCI_PRDT_ENTRY PRD[];
} AHCI_COMMAND_TABLE;
typedef struct _AHCI_CONTROLLER
{
	PCI_DEVICE      *DVC;
	AHCI_HBA_MEMORY *HBA;
} AHCI_CONTROLLER;
typedef struct _AHCI_PORT
{
	DISK_DRIVER DRV;
	AHCI_CONTROLLER *CTRL;
	AHCI_HBA_PORT *PRT;
	DWORD PNR;
	BYTE SER[0x14];
	BYTE MOD[0x28];
} AHCI_PORT;

void ConfigureAHCI(PCI_DEVICE *);

#endif