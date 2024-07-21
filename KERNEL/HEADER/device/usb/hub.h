#ifndef __KERNEL_DEVICE_USB_HUB_H__
#define __KERNEL_DEVICE_USB_HUB_H__

#include <device/usb.h>
#include <types.h>

#define USB_DT_HUB                      (USB_TYPE_CLASS | 0x09)
#define USB_DT_HUB3                     (USB_TYPE_CLASS | 0x0a)

#define USB_PORT_STAT_CONNECTION        0x0001
#define USB_PORT_STAT_ENABLE            0x0002
#define USB_PORT_STAT_SUSPEND           0x0004
#define USB_PORT_STAT_OVERCURRENT       0x0008
#define USB_PORT_STAT_RESET             0x0010
#define USB_PORT_STAT_POWER             0x0100
#define USB_PORT_STAT_LOW_SPEED         0x0200
#define USB_PORT_STAT_HIGH_SPEED        0x0400
#define USB_PORT_STAT_TEST              0x0800
#define USB_PORT_STAT_INDICATOR         0x1000

#pragma pack(push, 1)
typedef struct _USB_HUB_DESCRIPTOR
{
	BYTE DL; // Descriptor Length
	BYTE DT; // Descriptor Type;
	BYTE NP; // Port Number
	WORD HC; // Hub Characteristics;
	BYTE OG; // Power On to Power Good
	BYTE CP; // Power used by hub controller (mA)
	// Variable length fields
} USB_HUB_DESCRIPTOR;
#pragma pack(pop)
typedef struct _USB_PORT_STATUS
{
	WORD PS; // Port Status
	WORD PC; // Port Change
} USB_PORT_STATUS;

int ConfigureHUB(USB_COMMON *);
DWORD USBHUBDetect(USB_HUB *, DWORD);
DWORD USBHUBReset(USB_HUB *, DWORD);
DWORD USBHUBDisconnect(USB_HUB *, DWORD);

#endif // !__KERNEL_DEVICE_USB_HUB_H__
