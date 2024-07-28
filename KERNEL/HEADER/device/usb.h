#ifndef __KERNEL_DEVICE_USB_H__
#define __KERNEL_DEVICE_USB_H__

#include <types.h>
#include <device/disk.h>

#define USB_TYPE_UHCI  1
#define USB_TYPE_OHCI  2
#define USB_TYPE_EHCI  3
#define USB_TYPE_XHCI  4

#define USB_FULLSPEED  0
#define USB_LOWSPEED   1
#define USB_HIGHSPEED  2
#define USB_SUPERSPEED 3

#define USB_ENDPOINT_XFER_TYPE          0x03    /* in bmAttributes */
#define USB_ENDPOINT_XFER_CONTROL       0
#define USB_ENDPOINT_XFER_ISOC          1
#define USB_ENDPOINT_XFER_BULK          2
#define USB_ENDPOINT_XFER_INT           3
#define USB_ENDPOINT_DIR                0x80

#define USB_MAXADDR  127

#define USB_TIME_RSTRCY 10

#define USB_DIR_OUT                     0               /* to device */
#define USB_DIR_IN                      0x80            /* to host */

#define USB_TYPE_MASK                   (0x03 << 5)
#define USB_TYPE_STANDARD               (0x00 << 5)
#define USB_TYPE_CLASS                  (0x01 << 5)
#define USB_TYPE_VENDOR                 (0x02 << 5)
#define USB_TYPE_RESERVED               (0x03 << 5)

#define USB_RECIP_MASK                  0x1f
#define USB_RECIP_DEVICE                0x00
#define USB_RECIP_INTERFACE             0x01
#define USB_RECIP_ENDPOINT              0x02
#define USB_RECIP_OTHER                 0x03

#define USB_REQ_GET_STATUS              0x00
#define USB_REQ_CLEAR_FEATURE           0x01
#define USB_REQ_SET_FEATURE             0x03
#define USB_REQ_SET_ADDRESS             0x05
#define USB_REQ_GET_DESCRIPTOR          0x06
#define USB_REQ_SET_DESCRIPTOR          0x07
#define USB_REQ_GET_CONFIGURATION       0x08
#define USB_REQ_SET_CONFIGURATION       0x09
#define USB_REQ_GET_INTERFACE           0x0A
#define USB_REQ_SET_INTERFACE           0x0B
#define USB_REQ_SYNCH_FRAME             0x0C

#define USB_DT_DEVICE                   0x01
#define USB_DT_CONFIG                   0x02
#define USB_DT_STRING                   0x03
#define USB_DT_INTERFACE                0x04
#define USB_DT_ENDPOINT                 0x05
#define USB_DT_DEVICE_QUALIFIER         0x06
#define USB_DT_OTHER_SPEED_CONFIG       0x07
#define USB_DT_ENDPOINT_COMPANION       0x30

#define USB_CLASS_PER_INTERFACE         0x00       /* for DeviceClass */
#define USB_CLASS_AUDIO                 0x01
#define USB_CLASS_COMM                  0x02
#define USB_CLASS_HID                   0x03
#define USB_CLASS_PHYSICAL              0x05
#define USB_CLASS_STILL_IMAGE           0x06
#define USB_CLASS_PRINTER               0x07
#define USB_CLASS_MASS_STORAGE          0x08
#define USB_CLASS_HUB                   0x09

#define USB_INTERFACE_SUBCLASS_BOOT     1
#define USB_INTERFACE_PROTOCOL_KEYBOARD 1
#define USB_INTERFACE_PROTOCOL_MOUSE    2

#define US_PR_BULK         0x50  /* bulk-only transport */
#define US_PR_UAS          0x62  /* usb attached scsi   */

#define USB_CDB_SIZE 12

#define CBW_SIGNATURE 0x43425355 // USBC

struct _USB_COMMON;
struct _USB_HUB;
typedef struct _USB_PIPE USB_PIPE;
struct _USB_ENDPOINT;
struct _USB_CONTROLLER;
typedef struct _USB_DEVICE_REQUEST
{
	BYTE T; // Request Type
	BYTE C; // Request Code
	WORD V; // Value
	WORD I; // Index
	WORD L; // Length
} USB_DEVICE_REQUEST;
typedef struct _USB_DEVICE
{
	// Major data
	BYTE L;
	BYTE DT;   // Descriptor Type
	WORD VER;  // USB Version
	BYTE DC;   // Device Class
	BYTE DS;   // Device Subclass
	BYTE DP;   // Device Protocol
	BYTE MPS;  // Max Packet Size of endpoint 0
	// Minor data
	WORD VID;  // Vendor ID
	WORD PID;  // Product ID
	WORD DID;  // Device ID
	BYTE MIX;  // Manufacturer Index
	BYTE PIX;  // Product Index
	BYTE SIX;  // Serial Number Index
	BYTE CC;   // Configuration Count

} USB_DEVICE;
typedef struct _USB_CONFIG
{
	BYTE L;
	BYTE DT;   // Descriptor Type
	WORD TL;   // Total Length
	BYTE IC;   // Interface Count
	BYTE CV;   // Configuration Value
	BYTE CIX;  // Configuration Index
	BYTE AT;   // Attributes
	BYTE MP;   // Max Power
} USB_CONFIG;
typedef struct _USB_INTERFACE
{
	BYTE L;
	BYTE DT;   // Descriptor Type
	BYTE IN;   // Interface Number
	BYTE AS;   // Alternate Setting
	BYTE EC;   // Endpoint Count
	BYTE IC;   // Interface Class
	BYTE IS;   // Interface Subclass
	BYTE IP;   // Interface Protocol
	BYTE II;   // Interface Index
} USB_INTERFACE;
typedef struct _USB_ENDPOINT
{
	BYTE L;   // Length
	BYTE DT;  // DescriptorType
	BYTE EA;  // EndpointAddres
	BYTE AT;  // Attributes
	WORD MPS; // MaxPacketSize
	BYTE ITV; // Interval
} USB_ENDPOINT;
typedef struct _USB_HUB_OPERATION
{
	DWORD(*DTC)(struct _USB_HUB *, DWORD); // Detect
	DWORD(*RST)(struct _USB_HUB *, DWORD); // Reset
	DWORD(*DCC)(struct _USB_HUB *, DWORD); // Disconnect
} USB_HUB_OPERATION;
typedef struct _USB_HUB
{
	struct _USB_CONTROLLER *CTRL;
	struct _USB_COMMON *DEVICE;
	USB_HUB_OPERATION *OP;
	DWORD PC;
	DWORD DC;
} USB_HUB;
typedef struct _USB_PIPE
{
	struct _USB_CONTROLLER *CTRL;
	WORD MPS;
} USB_PIPE;
typedef struct _USB_COMMON
{
	struct _USB_COMMON *A0; // L
	struct _USB_COMMON *A1; // R
	struct _USB_CONTROLLER *CTRL;
	USB_CONFIG *CFG;
	USB_INTERFACE *IFC;
	USB_HUB *HUB;
	USB_PIPE *PIPE;
	DEVICE_DRIVER *DRV;
	DWORD PORT;
	DWORD SPD;
} USB_COMMON;
typedef struct _USB_CONTROLLER
{
	USB_HUB RH; // Root Hub
	struct _USB_PIPE *(*CPIP)(struct _USB_COMMON *, struct _USB_PIPE *, struct _USB_ENDPOINT *);
	DWORD (*XFER)(USB_PIPE *, USB_DEVICE_REQUEST *, void *, DWORD, DWORD);
	BYTE TYPE;
	BYTE MA; // Max Address
} USB_CONTROLLER;
typedef struct _USB_COMMAND_BLOCK_WRAPPER
{
	DWORD SIG;     // CBW Signature, fixed 0x43425355 ('USBC')
	DWORD TAG;     // CBW Identifier, device return it in CSW.dCSWTag
	DWORD DTL;     // CBW Required byte count while transfering
	BYTE  FLG;     // Data transfer direction, OUT=0x00, IN=0x80
	BYTE  LUN;     // LUN ID
	BYTE  CBL;     // Command length, in [0,16]
	BYTE  CMD[16]; // Command to transfer
} USB_COMMAND_BLOCK_WRAPPER;
#pragma pack(1)
typedef struct _USB_COMMAND_STATUS_WRAPPER
{
	DWORD SIG;
	DWORD TAG;
	DWORD RSD; // Remainder data size
	BYTE  STS; // Command status, 0 means correct
} USB_COMMAND_STATUS_WRAPPER;
#pragma pack()
typedef struct _USB_DISK_DRIVER
{
	DISK_DRIVER DVR;
	USB_PIPE   *BIP;
	USB_PIPE   *BOP;
	DWORD       LUN;
} USB_DISK_DRIVER;

extern const DWORD SPEED_TO_CTRL_SIZE[];
extern USB_CONTROLLER *USB_CTRL;

DWORD USBEnumerate(USB_HUB *);
DWORD USBSetAddress(USB_COMMON *);
DWORD ConfigureUSB(USB_COMMON *);
void USBD2P(USB_PIPE *, USB_COMMON *, USB_ENDPOINT *);
USB_ENDPOINT *USBSearchEndpoint(USB_COMMON *, int, int);

#endif
