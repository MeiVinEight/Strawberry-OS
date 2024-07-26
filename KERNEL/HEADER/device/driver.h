#ifndef __KERNEL_DEVICE_DRIVER_H__
#define __KERNEL_DEVICE_DRIVER_H__

#include <types.h>

#define DTYPE_USB_MSC          0x00
#define DTYPE_USB_MSC_32       0x01

typedef struct _DEVICE_DRIVER
{
	BYTE TYPE; // Driver type
} DEVICE_DRIVER;

#endif