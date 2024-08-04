#ifndef __KERNEL_DEVICE_SCSI_H__
#define __KERNEL_DEVICE_SCSI_H__

#include <device/disk.h>

#define SCSI_TYPE_DISK  0x00

#define CDB_CMD_TEST_UNIT_READY  0x00
#define CDB_CMD_REQUEST_SENSE    0x03
#define CDB_CMD_INQUIRY          0x12
#define CDB_CMD_READ_CAPACITY    0x25
#define CDB_CMD_READ_10          0x28
#define CDB_CMD_WRITE_10         0x2A
#define CDB_CMD_VERIFY_10        0x2F

#define SCSI_ASC_LOGICAL_UNIT_NOT_READY  0x04
#define SCSI_ASC_MEDIUM_NOT_PRESENT      0x3A

#define SCSI_ASQ_IN_PROGRESS             0x01

typedef struct _SCSI_DISK_DRIVER
{
	DISK_DRIVER DRV;
	BYTE MOD[0x18];
} SCSI_DISK_DRIVER;
typedef struct _SCSI_CMD_TEST_UNIT_READY
{
	BYTE CODE; // Operation Code = 0x00
	BYTE RSV0[4];
	BYTE CTRL;
	BYTE RSV1[10];
} SCSI_CMD_TEST_UNIT_READY;
typedef struct _SCSI_CMD_REQUEST_SENSE
{
	BYTE CODE      ; // Operation Code = 0x03
	BYTE DF  :0x01 ; // Descriptor Format
	BYTE RSV0:0x07 ;
	BYTE RSV1[0x02];
	BYTE AL        ; // Allocation Length
	BYTE CTRL      ;
	BYTE RSV2[0x0A];
} SCSI_CMD_REQUEST_SENSE;
#pragma pack(push, 1)
typedef struct _SCSI_CMD_INQUIRY
{
	BYTE CODE:0x08;
	BYTE EVPD:0x01;
	BYTE CSD :0x01; // Command Support Data
	BYTE RSV0:0x06;
	BYTE PC  :0x08; // Page Code
	WORD AL  ;      // Allocation Length
	BYTE CTRL;      // Control
	BYTE PAD [0x0A];
} SCSI_CMD_INQUIRY;
#pragma pack(pop)
#pragma pack(push, 1)
typedef struct _SCSI_CMD_READ_CAPACITY
{
	BYTE  CODE      ; // Operation Code = 0x25
	BYTE  OBS :0x01 ; // Obsolete
	BYTE  RSV0:0x07 ;
	DWORD LBA       ; // Logical Block Address
	BYTE  RSV1[0x02];
	BYTE  PMI :0x01 ; // Partial Medium Indicator
	BYTE  RSV2:0x07 ;
	BYTE  CTRL      ;
	BYTE  RSV3[0x06];
} SCSI_CMD_READ_CAPACITY;
#pragma pack(pop)
#pragma pack(push, 1)
typedef struct _SCSI_CMD_READ_10
{
	BYTE  CODE      ; // Operation Code = 0x28
	BYTE  OBS :0x02 ;
	BYTE  RARC:0x01 ; // Rebuild assist recovery control
	BYTE  FUA :0x01 ; // Force unit Access
	BYTE  DPO :0x01 ; // Disable Page Out
	BYTE  RDPT:0x03 ; // Read Protect
	DWORD LBA       ; // Logical Block Address
	BYTE  GN  :0x05 ; // Group Number
	BYTE  RSV0:0x03 ;
	WORD  TL        ; // Transfer Length
	BYTE  CTRL      ;
	BYTE  RSV1[0x06];
} SCSI_CMD_READ_10;
#pragma pack(pop)
#pragma pack(push, 1)
typedef struct _SCSI_CMD_WRITE_10
{
	BYTE  CODE      ; // Operation Code = 0x2A
	BYTE  OBS :0x02 ;
	BYTE  RSV0:0x01 ;
	BYTE  FUA :0x01 ; // Force unit Access
	BYTE  DPO :0x01 ; // Disable Page Out
	BYTE  WRPT:0x03 ; // Write Protect
	DWORD LBA       ; // Logical Block Address
	BYTE  GN  :0x05 ; // Group Number
	BYTE  RSV1:0x03 ;
	WORD  TL        ; // Transfer Length
	BYTE  CTRL      ;
	BYTE  RSV2[0x06];
} SCSI_CMD_WRITE_10;
#pragma pack(pop)
#pragma pack(push, 1)
typedef struct _SCSI_DAT_REQUEST_SENSE
{
	BYTE  CODE:0x07 ; // Response Code
	BYTE  VLD :0x01 ; // Valid
	BYTE  OBS       ; // Obsolete
	BYTE  KEY :0x04 ; // Sense Key
	BYTE  RSV0:0x01 ;
	BYTE  ILI :0x01 ; // Incorrect length indicator
	BYTE  EOM :0x01 ; // End-of-Medium
	BYTE  FM  :0x01 ; // File Mask
	DWORD INFO      ;
	BYTE  ASL       ; // Additional Sense Length
	DWORD CSI       ; // Command Specific Information
	BYTE  ASC       ; // Additional Sense Code
	BYTE  ASQ       ; // Additional Sense Qualifier
	BYTE  FRUC      ; // Field Replaceable Unit Code
	BYTE  SKS0:0x07 ; // Sense Key Specific high 7 bit
	BYTE  SKSV:0x01 ; // Sense-key Specific Valid
	WORD  SKS1      ; // Sense Key Specific low 16 bit
	BYTE  DATA[    ]; // Additional Sense Bytes
} SCSI_DAT_REQUEST_SENSE;
#pragma pack(pop)
typedef struct _SCSI_DAT_INQUIRY
{
	BYTE PDT :0x05; // Peripheral Device Type
	BYTE PQ  :0x03; // Peripheral Qualifier

	BYTE RSV0:0x07;
	BYTE RMV :0x01; // Removable

	BYTE VER ;      // Version

	BYTE RDF :0x04; // Response Data Format
	BYTE HAC :0x01; // Hierarchical Support
	BYTE ACA :0x01; // Normal ACA Support
	BYTE RSV1:0x02;

	BYTE AL  ;      // Additional Length

	BYTE PRT :0x01; // Protect
	BYTE RSV2:0x02;
	BYTE TPC :0x01; // Third Party Copy
	BYTE TPG :0x02; // Target Port Group Support
	BYTE ACC :0x01; // Access Controls Coordinator
	BYTE SCC :0x01; // SCC Support

	BYTE RSV3:0x04;
	BYTE MP  :0x01; // Multi Port
	BYTE VS0 :0x01;
	BYTE ES  :0x01; // Enclosure Service
	BYTE RSV4:0x01;

	BYTE VS1 :0x01;
	BYTE CQ  :0x01; // Command Queuing
	BYTE RSV5:0x06;

	BYTE VEN [0x08];// T10 Vendor Identification
	BYTE PROD[0x10];// Product Identification
	BYTE REV [0x04];// Product Revision Identification
	BYTE SRA [0x08];// Drive Serial Number
	// BYTE VUS [0x0C];// Vendor Unique Seagate
	// WORD RSV6;
	// WORD VERD[0x08];//Version Descriptors
} SCSI_DAT_INQUIRY;
typedef struct _SCSI_DAT_READ_CAPACITY
{
	DWORD LBA;
	DWORD BSZ;
} SCSI_DAT_READ_CAPACITY;

int SCSISetup(SCSI_DISK_DRIVER *);
int SCSISetupCommand(DISK_OPERATION *, void *, int);

#endif