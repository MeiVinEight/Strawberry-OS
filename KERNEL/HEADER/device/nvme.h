#ifndef __KERNEL_DEVICE_NVME_H__
#define __KERNEL_DEVICE_NVME_H__

#include <device/pci.h>
#include <device/disk.h>

typedef struct _NVME_CONTROLLER NVME_CONTROLLER;
typedef struct _NVME_CAPABILITY
{
	QWORD CAP ; // Controller Capabilities
	DWORD VER ; // Version
	DWORD IMS ; // Interrupt Mask Set
	DWORD IMC ; // Interrupt Mask Clear
	DWORD CC  ; // Controller Configuration
	DWORD RSV0;
	DWORD CST ; // Controller Status
	DWORD RSV1;
	DWORD AQA ; // Admin Queue Attributes
	QWORD ASQ ; // Admin Submission Queue
	QWORD ACQ ; // Admin Completion Queue
} NVME_CAPABILITY;
typedef struct _NVME_SUBMISSION_QUEUE_ENTRY
{
	DWORD CDW0;
	DWORD NSID; // Namespace ID
	DWORD CDW2;
	DWORD CDW3;
	QWORD META; // Metadata
	QWORD DATA[2]; // Data
	DWORD CDWA;
	DWORD CDWB;
	DWORD CDWC;
	DWORD CDWD;
	DWORD CDWE;
	DWORD CDWF;
} NVME_SUBMISSION_QUEUE_ENTRY;
typedef struct _NVME_COMPLETION_QUEUE_ENTRY
{
	DWORD DW0;
	DWORD DW1;
	WORD  QHD; // SQ Head Pointer
	WORD  QID; // SQ ID
	WORD  CID; // Command ID
	WORD  STS; // Status
} NVME_COMPLETION_QUEUE_ENTRY;
typedef struct _NVME_QUEUE_COMMON
{
	NVME_CONTROLLER *CTR;
	DWORD *DBL;
	WORD   MSK;
	WORD   HAD;
	WORD   TAL;
	WORD   PHA;
} NVME_QUEUE_COMMON;
typedef struct _NVME_COMPLETION_QUEUE
{
	NVME_QUEUE_COMMON COM;
	NVME_COMPLETION_QUEUE_ENTRY *CQE;
} NVME_COMPLETION_QUEUE;
typedef struct _NVME_SUBMISSION_QUEUE
{
	NVME_QUEUE_COMMON COM;
	NVME_SUBMISSION_QUEUE_ENTRY *SQE;
	NVME_COMPLETION_QUEUE *ICQ;
} NVME_SUBMISSION_QUEUE;
typedef struct _NVME_CONTROLLER
{
	PCI_DEVICE *DVC;
	NVME_CAPABILITY *CAP;
	QWORD DST; // Doorbell Stride
	DWORD WTO; // Waiting Timeout (ms)
	DWORD NSC; // Namespace Count
	NVME_COMPLETION_QUEUE ACQ;
	NVME_SUBMISSION_QUEUE ASQ;
	NVME_COMPLETION_QUEUE ICQ;
	NVME_SUBMISSION_QUEUE ISQ;
} NVME_CONTROLLER;
typedef struct _NVME_LOGICAL_BLOCK_ADDRESS
{
	WORD MS; // Metadata Size
	BYTE DS; // LBA Data Size
	BYTE RP; // Relative Performance
} NVME_LOGICAL_BLOCK_ADDRESS;
typedef struct _NVME_IDENTIFY_NAMESPACE
{
	QWORD SIZE; // Namespace Size
	QWORD CAPA; // Namespace Capacity
	QWORD UTIL; // Namespace Utilization
	BYTE  FEAT; // Namespace Features
	BYTE  NLBA; // Number of LBA Format
	BYTE  FLBA; // Formatted LBA size
	BYTE  META; // Metadata Capability
	BYTE  DPCA; // End-to-end Data Protection Capability
	BYTE  DPTS; // End-to-end Data Protection Type Settings
	BYTE  NMIC; // Namespace Multi-path I/O and Namespace Sharing Capabilities
	BYTE  RSVC; // Reservation Capabilities
	BYTE  FPID; // Format Prograss Indicator
	BYTE  DLBF; // Deallocate Logical Block Feature
	WORD  AWUN; // Namespace Atomic Write Unit Normal
	WORD  AWUP; // Namespace Atomic Write Unit Power Fail
	WORD  ACWU; // Namespace Atomic Compare & Write Unit
	WORD  ABSN; // Namespace Atomic Boundary Size Normal
	WORD  ABOS; // Namespace Atomic Boundary Offset
	WORD  ABSP; // Namespace Atomic Boundary Size Power Fail
	WORD  OIOB; // Namespace Optimal I/O Boundary
	BYTE  NVMC[16]; // NVM Capacity
	WORD  NPWG; // Namespace Preferred Write Granularity
	WORD  NPWA; // Namespace Preferred Write Alignment
	WORD  NPDG; // Namespace Preferred Deallocate Granularity
	WORD  NPDA; // Namespace Preferred Deallocate Alignment
	WORD  NOWS; // Namespace Optimal Write Size
	WORD  SSRL; // Maximum Single Source Range Length
	DWORD MXCL; // Maximum Copy Length
	BYTE  MSRC; // Maximum Source Range Count (MSRC)
	BYTE  RSV0[11];
	DWORD AGID; // ANA Group Identifier
	BYTE  RSV1[3];
	BYTE  ATTR; // Namespace Attributes
	WORD  NSID; // NVM Set Identifier
	WORD  EGID; // Endurance Group Identifier
	QWORD GUID[2]; // Namespace Globally Unique Identifier
	QWORD XUID; // IEEE Extended Unique Identifier
	NVME_LOGICAL_BLOCK_ADDRESS LBAF[64];
} NVME_IDENTIFY_NAMESPACE;
typedef struct _NVME_IDENTIFY_CONTROLLER
{
	WORD  PVID; // PCI Vendor ID
	WORD  PSID; // PCI Subsystem Vendor ID
	BYTE  SERN[20]; // Serial Number
	BYTE  MODN[40]; // Model Number
	BYTE  FREV[8]; // Frimware Revision
	BYTE  RCAB; // Recommended Arbitration Burst
	BYTE  IEEE[3]; // IEEE OUI Identifier
	BYTE  CMIC; // Controller Multi-Path I/O and Namespace Sharing Capabilities
	BYTE  MDTS; // Maximum Data Transfer Size
	WORD  CTID; // Controller ID
	DWORD VERS; // Version
	DWORD RTDR; // RTD3 Resume Latency
	DWORD RTDE; // RTD3 Entry Latency
	DWORD OAES; // Optional Asynchronous Events Supported
	DWORD CRTA; // Controller Attributes
	WORD  RRLS; // Read Recovery Levels Supported
	BYTE  RSV0[9];
	BYTE  CTRT; // Controller Type
	QWORD GUID[2]; // FRU Globally Unique Identifier
	WORD  CRD1; // Command Retry Delay Time 1
	WORD  CRD2; // Command Retry Delay Time 2
	WORD  CRD3; // Command Retry Delay Time 3
	BYTE  RSV1[119];
	BYTE  NVMR; // NVM Subsystem Report
	BYTE  VWCI; // VPD Write Cycle Information
	BYTE  MECA; // Management Endpoint Capabilities
	WORD  OACS; // Optional Admin Command Support
	BYTE  ACLM; // Abort Command Limit
	BYTE  AERL; // Asynchronous Event Request Limit
	BYTE  FRMW; // Firmware Updates
	BYTE  LPGA; // Log Page Attributes
	BYTE  ELPE; // Error Log Page Entries
	BYTE  NPSS; // Number of Power States Support
	BYTE  AVCC; // Admin Vendor Specific Command Configuration
	BYTE  APST; // Autonomous Power State Transition Attributes
	WORD  WCTT; // Warning Composite Temperature Threshold
	WORD  CCTT; // Critical Composite Temperature Threshold
	WORD  MTFA; // Maximum Time for Firmware Activation
	DWORD HMPS; // Host Memory Buffer Preferred Size
	DWORD HMMS; // Host Memory Buffer Minimum Size
	QWORD TNVM[2]; // Total NVM Capacity
	QWORD UNVM[2]; // Unallocated NVM Capacity
	DWORD RPMB; // Replay Protected Memory Block Support
	WORD  XDST; // Extended Device Self-test Time
	BYTE  DSTO; // Device Self-test Options
	BYTE  FWUG; // Firmware Update Granularity
	WORD  KALV; // Keep Alive Support
	WORD  HCTM; // Host Controlled Thermal Management Attributes
	WORD  MNTM; // Minimum Thermal Management Temperature
	WORD  MXTM; // Maximum Thermal Management Temperature
	DWORD SANC; // Sanitize Capabilities
	DWORD MNDS; // Host Memory Buffer Minimum Descriptor Entry Size
	WORD  MXDE; // Host Memory Maximum Descriptors Entries
	WORD  NSIM; // NVM Set Identifier Maximum
	WORD  EGIM; // Endurance Group Identifier Maximum
	BYTE  ANAT; // ANA Transition Time
	BYTE  ANAC; // Asymmetric Namespace Access Capabilities
	DWORD AGIM; // ANA Group Identifier Maximum
	DWORD AGIN; // Number of ANA Group Identifiers
	DWORD PELS; // Persistent Event Log Size
	WORD  DMID; // Domain Identifier
	BYTE  RSV2[10];
	QWORD MEGC[2]; // Max Endurance Group Capacity
	BYTE  RSV3[128];
	BYTE  SQES; // Submission Queue Entry Size
	BYTE  CQES; // Completion Queue Entry Size
	WORD  MXOC; // Maximum Outstanding Commands
	DWORD NNAM; // Number of Namespaces
} NVME_IDENTIFY_CONTROLLER;
typedef struct _NVME_IDENTIFY_NAMESPACE_LIST
{
	DWORD NSID[1024];
} NVME_IDENTIFY_NAMESPACE_LIST;
typedef struct _NVME_NAMESPACE
{
	DISK_DRIVER DRV;
	NVME_CONTROLLER *CTRL;
	QWORD NLBA; // Number of LBA
	DWORD NSID;
	DWORD BSZ;
	DWORD META;
	DWORD MXRS; // Max Request Size
} NVME_NAMESPACE;

void ConfigureNVME(PCI_DEVICE *);

#endif