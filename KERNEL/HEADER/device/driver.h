#ifndef __KERNEL_DEVICE_DRIVER_H__
#define __KERNEL_DEVICE_DRIVER_H__

#include <types.h>

typedef struct _DEVICE_DRIVER
{
	BYTE TYPE; // Driver type
	BYTE RMV;  // Is media removable (currently unused)
} DEVICE_DRIVER;

#endif