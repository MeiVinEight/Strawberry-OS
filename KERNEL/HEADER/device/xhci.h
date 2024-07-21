#ifndef __KERNEL_DEVICE_XHCI_H__
#define __KERNEL_DEVICE_XHCI_H__

#include <device/pci.h>
#include <device/usb.h>

#define XHCI_TIME_POSTPOWER      20
#define XHCI_RING_ITEMS          255

#define XHCI_CMD_RS              (1<< 0) // Run Stop
#define XHCI_CMD_HCRST           (1<< 1)
#define XHCI_CMD_INTE            (1<< 2)
#define XHCI_CMD_HSEE            (1<< 3)
#define XHCI_CMD_LHCRST          (1<< 7)
#define XHCI_CMD_CSS             (1<< 8)
#define XHCI_CMD_CRS             (1<< 9)
#define XHCI_CMD_EWE             (1<<10)
#define XHCI_CMD_EU3S            (1<<11)

#define XHCI_STS_HCH             (1<< 0)
#define XHCI_STS_HSE             (1<< 2)
#define XHCI_STS_EINT            (1<< 3)
#define XHCI_STS_PCD             (1<< 4)
#define XHCI_STS_SSS             (1<< 8)
#define XHCI_STS_RSS             (1<< 9)
#define XHCI_STS_SRE             (1<<10)
#define XHCI_STS_CNR             (1<<11)
#define XHCI_STS_HCE             (1<<12)

#define XHCI_PORTSC_CCS          (1<< 0) // Current Connect Status
#define XHCI_PORTSC_PED          (1<< 1) // Port Enabled Disabled
#define XHCI_PORTSC_PR           (1<< 4) // Port Reset
#define XHCI_PORTSC_PP           (1<< 9) // Port Power

#define TRB_TR_IDT          (1<<6)

#define TRB_LINK_TC 2

typedef struct _XHCI_CAPABILITY
{
	BYTE  CL;   // Capability Register Length
	BYTE  RSV;  // Reserved
	WORD  IVN;  // Interface Version Number
	DWORD SP1;  // Struct Parameters 1
	DWORD SP2;  // Struct Parameters 2
	DWORD SP3;  // Struct Parameters 3
	DWORD CP1;  // Capability Parameters
	DWORD DBO;  // Doorbell Offset
	DWORD RRSO; // Runtime Registers Space Offset
	DWORD CP2;  // Capability Parameters 2
} XHCI_CAPABILITY;
typedef struct _XHCI_OPERATIONAL
{
	DWORD CMD;     // USB Command
	DWORD STS;     // USB Status
	DWORD PS;      // Page Size
	DWORD RSV0[2];
	DWORD DNC;     // Device Notification Control
	QWORD CRC;     // Command Ring Control
	DWORD RSV1[4];
	QWORD DCA;     // Device Context Base Address Array Pointer
	DWORD CFG;     // Configure
} XHCI_OPERATIONAL;
typedef struct _XHCI_PORT
{
	DWORD PSC;   // Port Status and Control
	DWORD PPMSC; // Port Power Management Status and Control
	DWORD PLI;   // Port Link Info
	DWORD RSV;
} XHCI_PORT;
typedef struct _XHCI_INTERRUPT
{
	DWORD IMAN; // Interrupt Management
	DWORD IMOD; // Interrupt Moderation
	DWORD TS;   // Event Ring Segment Table Size
	DWORD RSV;
	QWORD ERS;  // Event Ring Segment Table Base Address
	QWORD ERD;  // Event Ring Dequeue Pointer
} XHCI_INTERRUPT;
typedef struct _XHCI_RUNTIME
{
	QWORD MFI[4];         // Microframe Index
	XHCI_INTERRUPT IR[0]; // Interrupt Register Set
} XHCI_RUNTIME;
typedef struct _XHCI_XCAPABILITY
{
	DWORD CAP;
	DWORD DATA[];
} XHCI_XCAPABILITY;
typedef union _XHCI_TRANSFER_BLOCK // transfer block (ring element)
{
	struct
	{
		DWORD DATA[4];
	};
	struct
	{
		DWORD RSV0;
		DWORD RSV1;
		DWORD RSV2;
		DWORD C   :0x01; // Cycle
		DWORD RSV3:0x09;
		DWORD TYPE:0x06; // TBR Type
		DWORD RSV4:0x10;
	};
} XHCI_TRANSFER_BLOCK;
typedef struct _XHCI_TRANSFER_RING
{
	XHCI_TRANSFER_BLOCK   *RING;
	XHCI_TRANSFER_BLOCK    EVT;
	WORD                   EID;
	WORD                   NID;
	BYTE                   CCS;
	BYTE                   LOCK;
} XHCI_TRANSFER_RING;
// event ring segment
typedef struct _XHCI_RING_SEGMENT
{
	QWORD A; // Address
	DWORD S; // Size
	DWORD R; // Reserved
} XHCI_RING_SEGMENT;
typedef struct _XHCI_CONTROLLER
{
	USB_CONTROLLER USB;
	// Register Set
	XHCI_CAPABILITY  *CR; // Capability Registers
	XHCI_OPERATIONAL *OR; // Operational Registers
	XHCI_PORT        *PR; // Port Registers
	XHCI_RUNTIME     *RR; // Runtime Registers
	DWORD            *DR; // Doorbell Registers
	// Data structures
	QWORD             *DVC; // Device Context List
	XHCI_RING_SEGMENT *SEG; // Device Segment List
	XHCI_TRANSFER_RING CMD; // Device Command List
	XHCI_TRANSFER_RING EVT; // Device Event List
	// HCSP1
	DWORD SN; // Number of Device Slots (MAXSLOTS)
	DWORD IN; // Number of Interrupts (MAXINTRS)
	DWORD PN; // Number of Ports (MAXPORTS)
	// HCCP1
	DWORD XEC; // xHCI Extended Capability Pointer
	DWORD CSZ; // Context Size
} XHCI_CONTROLLER;
typedef struct _XHCI_INPUT_COMTROL_CONTEXT
{
	DWORD DRP;
	DWORD ADD;
	DWORD RSV0[5];
	BYTE  CV; // Configuration Value
	BYTE  IN; // Interface Number
	BYTE  AS; // Alternate Setting
	BYTE  RSV1;
} XHCI_INPUT_CONTROL_CONTEXT;
typedef struct _XHCI_SLOT_CONTEXT
{
	DWORD RS  :0x14; // Route String
	DWORD SPD :0x04; // Speed
	DWORD RSV0:0x01;
	DWORD MTT :0x01; // Multi-TT
	DWORD HUB :0x01;
	DWORD CE  :0x05; // Context Entries
	WORD  MEL      ; // Max Exit Latency
	BYTE  RHPN     ; // Root Hub Port Number
	BYTE  PN       ; // Number of Ports
	BYTE  TTID     ; // TT Hub Slot ID
	BYTE  TTP      ; // TT Port Number
	WORD  TTT :0x02; // TT Think Time
	WORD  RSV1:0x04;
	WORD  IT  :0x0A; // Interrupter Target
	DWORD DA  :0x08; // USB Device Address
	DWORD RSV2:0x13;
	DWORD SS  :0x05; // Slot State
	DWORD RSV3[4];
} XHCI_SLOT_CONTEXT;
typedef struct _XHCI_ENDPOINT_CONTEXT
{
	DWORD EPS :0x03; // EP State
	DWORD RSV0:0x05;
	DWORD MULT:0x02;
	DWORD MPSM:0x05; // Max Primary Streams
	DWORD LSA :0x01; // Linear Stream Array
	DWORD ITV :0x08; // Interval
	DWORD MEPH:0x08; // Max Endpoint Service Time Interval Payload Hi

	DWORD RSV1:0x01;
	DWORD EC  :0x02; // Error Count
	DWORD EPT :0x03; // Endpoint Type
	DWORD RSV2:0x01;
	DWORD HID :0x01; // Host Initiate Disable
	DWORD MBS :0x08; // Max Brust Size
	DWORD MPSZ:0x10; // Max Packet Size

	QWORD DCS :0x01; // Dequeue Cycle State
	QWORD RSV3:0x03;
	QWORD TRDP:0x3C; // TR Dequeue Pointer

	WORD  ATL      ; // Average TRB Length
	WORD  MEPL     ; // Max ESIT Payload Lo
	DWORD RSV4[3];
} XHCI_ENDPOINT_CONTEXT;
typedef struct _XHCI_PIPE
{
	XHCI_TRANSFER_RING RING;
	USB_PIPE USB;
	DWORD SID;
	DWORD EPID;
} XHCI_PIPE;
typedef union _XHCI_TRB_SETUP_STAGE
{
	XHCI_TRANSFER_BLOCK TRB;
	struct
	{
		USB_DEVICE_REQUEST DATA;

		DWORD TL  :0x11; // TRB Transfer Length
		DWORD RSV0:0x05;
		DWORD IT  :0x0A; // Interrupt Target

		DWORD C   :0x01; // Cycle
		DWORD RSV1:0x04;
		DWORD IOC :0x01; // Interrupt On Completion
		DWORD IDT :0x01; // Immediate Data
		DWORD RSV2:0x03;
		DWORD TYPE:0x06; // TRB Type
		DWORD TRT :0x02; // Transfer Type
		DWORD RSV3:0x0E;
	};
} XHCI_TRB_SETUP_STAGE;
typedef union _XHCI_TRB_DATA_STAGE
{
	XHCI_TRANSFER_BLOCK TRB;
	struct
	{
		QWORD DATA:0x40; // Data Buffer

		DWORD TL  :0x11; // Transfer Length
		DWORD TDS :0x05; // TD Size
		DWORD IT  :0x0A; // Interrupt Target

		DWORD C   :0x01; // Cycle
		DWORD ENT :0x01; // Evaluate Next TRB
		DWORD ISP :0x01; // Interrupt on Short Packet
		DWORD NS  :0x01; // No Snoop
		DWORD CH  :0x01; // Chain
		DWORD IOC :0x01; // Interrupt On Completion
		DWORD IDT :0x01; // Immediate Data
		DWORD RSV0:0x03;
		DWORD TYPE:0x06; // TRB Type
		DWORD DIR :0x01; // Direction
		DWORD RSV1:0x0F;
	};
} XHCI_TRB_DATA_STAGE;
typedef union _XHCI_TRB_STATUS_STAGE
{
	XHCI_TRANSFER_BLOCK TRB;
	struct
	{
		DWORD RSV0;

		DWORD RSV1;

		DWORD RSV2:0x16;
		DWORD IT  :0x0A; // Interrupt Target

		DWORD C   :0x01; // Cycle
		DWORD ENT :0x01; // Evaluate Next TRB
		DWORD RSV3:0x02;
		DWORD CH  :0x01; // Chain
		DWORD IOC :0x01; // Interrupt On Completion
		DWORD RSV4:0x04;
		DWORD TYPE:0x06; // TRB Type
		DWORD DIR :0x01; // Direction
		DWORD RSV5:0x0F;
	};
} XHCI_TRB_STATUS_STAGE;
typedef union _XHCI_TRB_LINK
{
	XHCI_TRANSFER_BLOCK TRB;
	struct
	{
		QWORD RSV0:0x04;
		QWORD RSP :0x3C; // Ring Segment Pointer
		DWORD RSV1:0x16;
		DWORD IT  :0x0A; // Interrupter Target
		DWORD C   :0x01; // Cycle
		DWORD TC  :0x01; // Toggle Cycle
		DWORD RSV2:0x02;
		DWORD CH  :0x01; // Chain
		DWORD IOC :0x01; // Interrupt On Completion
		DWORD RSV3:0x04;
		DWORD TYPE:0x06; // TRB Type
		DWORD RSV4:0x10;
	};
} XHCI_TRB_LINK;
typedef union _XHCI_TRB_ENABLE_SLOT
{
	XHCI_TRANSFER_BLOCK TRB;
	struct
	{
		DWORD RSV0[3];
		DWORD C   :0x01; // Cycle
		DWORD RSV1:0x09;
		DWORD TYPE:0x06; // TBR Type
		DWORD ST  :0x05; // Slot Type
		DWORD RSV2:0x0B;
	};
} XHCI_TRB_ENABLE_SLOT;
typedef union _XHCI_TRB_DISABLE_SLOT
{
	XHCI_TRANSFER_BLOCK TRB;
	struct
	{
		DWORD RSV0[3];
		WORD  C   :0x01; // Cycle
		WORD  RSV1:0x09;
		WORD  TYPE:0x06; // TRB Type
		BYTE  RSV2;
		BYTE  SID ;
	};
} XHCI_TRB_DISABLE_SLOT;
typedef union _XHCI_TRB_ADDRESS_DEVICE
{
	XHCI_TRANSFER_BLOCK TRB;
	struct
	{
		QWORD RSV0:0x04;
		QWORD ICP :0x3C; // Input Context Pointer
		DWORD RSV1;
		WORD  C   :0x01; // Cycle
		WORD  RSV2:0x08;
		WORD  BSR :0x01; // Block Set Address Request
		WORD  TYPE:0x06; // TRB Type
		BYTE  RSV3;
		BYTE  SID ;      // Slot ID
	};
} XHCI_TRB_ADDRESS_DEVICE;
typedef union _XHCI_TRB_CONFIGURE_ENDPOINT
{
	XHCI_TRANSFER_BLOCK TRB;
	struct
	{
		QWORD ICP :0x40; // Input Context Pointer (0x10 aligned)
		DWORD RSV0;
		WORD  C   :0x01; // Cycle
		WORD  RSV1:0x08;
		WORD  DC  :0x01; // Deconfigure
		WORD  TYPE:0x06; // TRB Type
		BYTE  RSV2;
		BYTE  SID ;      // Slot ID
	};
} XHCI_TRB_CONFIGURE_ENDPOINT;
typedef union _XHCI_TRB_EVALUATE_CONTEXT
{
	XHCI_TRANSFER_BLOCK TRB;
	struct
	{
		QWORD ICP ;      // Input Context Pointer (0x16 aligned)
		DWORD RSV0;
		WORD  C   :0x01; // Cycle
		WORD  RSV1:0x08;
		WORD  BSR :0x01; // Block Set Address Request
		WORD  TYPE:0x06; // TRB Type
		BYTE  RSV2;
		BYTE  SID ;      // Slot ID
	};
} XHCI_TRB_EVALUATE_CONTEXT;
typedef union _XHCI_TRB_COMMAND_COMPLETION
{
	XHCI_TRANSFER_BLOCK TRB;
	struct
	{
		QWORD CTP :0x40; // Command TRBV Pointer
		DWORD CCP :0x18; // Command Completion Parameter
		DWORD CC  :0x08; // Completion Code
		WORD  C   :0x01; // Cycle
		WORD  RSV1:0x09;
		WORD  TYPE:0x06;
		BYTE  VFID;
		BYTE  SID ;
	};
} XHCI_TRB_COMMAND_COMPLETION;
typedef union _XHCI_TRB_TRANSFER_RING
{
	XHCI_TRANSFER_BLOCK TRB;
	struct
	{
		QWORD RING;
		DWORD RSV0;
		WORD  C   :0x01; // Cycle
		WORD  RSV1:0x09;
		WORD  TYPE:0x06; // TRB Type
		WORD  RSV2;
	};
} XHCI_TRB_TRANSFER_RING;
typedef enum _XHCI_PORT_LINK_STATE
{
	PLS_U0 = 0,
	PLS_U1 = 1,
	PLS_U2 = 2,
	PLS_U3 = 3,
	PLS_DISABLED = 4,
	PLS_RX_DETECT = 5,
	PLS_INACTIVE = 6,
	PLS_POLLING = 7,
	PLS_RECOVERY = 8,
	PLS_HOT_RESET = 9,
	PLS_COMPILANCE_MODE = 10,
	PLS_TEST_MODE = 11,
	PLS_RESUME = 15,
} XHCI_PORT_LINK_STATE;
typedef enum _XHCI_ENDPOINT_TYPE
{
	EP_NOT_VALID = 0,
	EP_OUT_ISOCH,
	EP_OUT_BULK,
	EP_OUT_INTERRUPT,
	EP_CONTROL,
	EP_IN_ISOCH,
	EP_IN_BULK,
	EP_IN_INTERRUPT
} XHCI_ENDPOINT_TYPE;
typedef enum _XHCI_TRB_TYPE
{
	TRB_RESERVED = 0,
	TRB_NORMAL,
	TRB_SETUP_STAGE,
	TRB_DATA_STAGE,
	TRB_STATUS_STAGE,
	TRB_ISOCH,
	TRB_LINK,
	TRB_EVDATA,
	TRB_NOOP,
	TRB_ENABLE_SLOT,
	TRB_DISABLE_SLOT,
	TRB_ADDRESS_DEVICE,
	TRB_CONFIGURE_ENDPOINT,
	TRB_EVALUATE_CONTEXT,
	TRB_RESET_ENDPOINT,
	TRB_STOP_ENDPOINT,
	TRB_SET_TR_DEQUEUE,
	TRB_RESET_DEVICE,
	TRB_FORCE_EVENT,
	TRB_NEGOTIATE_BW,
	TRB_SET_LATENCY_TOLERANCE,
	TRB_GET_PORT_BANDWIDTH,
	TRB_FORCE_HEADER,
	TRB_NOOP_COMMAND,
	TRB_TRANSFER = 32,
	TRB_COMMAND_COMPLETE,
	TRB_PORT_STATUS_CHANGE,
	TRB_BANDWIDTH_REQUEST,
	TRB_DOORBELL,
	TRB_HOST_CONTROLLER,
	TRB_DEVICE_NOTIFICATION,
	TRB_MFINDEX_WRAP,
} XHCI_TRB_TYPE;

extern const DWORD SPEED_XHCI[];

void SetupXHCI();
void SetupXHCIControllerPCI(PCI_DEVICE *);
XHCI_CONTROLLER *SetupXHCIController(QWORD);
DWORD ConfigureXHCI(XHCI_CONTROLLER *);
void XHCIQueueTRB(XHCI_TRANSFER_RING *, XHCI_TRANSFER_BLOCK *);
void XHCICopyTRB(XHCI_TRANSFER_RING *, XHCI_TRANSFER_BLOCK *);
void XHCIDoorbell(XHCI_CONTROLLER *, DWORD, DWORD);
void XHCIProcessEvent(XHCI_CONTROLLER *);
DWORD XHCIWaitCompletion(XHCI_CONTROLLER *, XHCI_TRANSFER_RING *);
DWORD XHCICommand(XHCI_CONTROLLER *, XHCI_TRANSFER_BLOCK *);
DWORD XHCIHUBDetect(USB_HUB *, DWORD);
DWORD XHCIHUBReset(USB_HUB *, DWORD);
DWORD XHCIHUBDisconnect(USB_HUB *, DWORD);
USB_PIPE *XHCICreatePipe(USB_COMMON *, USB_PIPE *, USB_ENDPOINT *);
DWORD XHCITransfer(USB_PIPE *, USB_DEVICE_REQUEST *, void *);
void XHCICreateTransferRing(XHCI_TRANSFER_RING *);

#endif