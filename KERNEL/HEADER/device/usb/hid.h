#ifndef __KERNEL_DEVICE_USB_HID_H__
#define __KERNEL_DEVICE_USB_HID_H__

#include <types.h>
#include <device/usb.h>

DWORD ConfigureHID(USB_COMMON *);
void USBKeyboardEvent();

#endif