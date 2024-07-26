#ifndef __KERNEL_PCI_H__
#define __KERNEL_PCI_H__

#include <types.h>

#define PCI_CONFIG_ADDRESS 0x0CF8
#define PCI_CONFIG_DATA    0x0CFC

#define PCI_COMMAND         0x04	/* 16 bits */
#define PCI_BASE_ADDRESS_0	0x10	/* 32 bits */
#define PCI_BASE_ADDRESS_1	0x14	/* 32 bits [htype 0,1 only] */
#define PCI_BASE_ADDRESS_2	0x18	/* 32 bits [htype 0 only] */
#define PCI_BASE_ADDRESS_3	0x1c	/* 32 bits */
#define PCI_BASE_ADDRESS_4	0x20	/* 32 bits */
#define PCI_BASE_ADDRESS_5	0x24	/* 32 bits */

#define PCI_COMMAND_MEMORY  0x2	/* Enable response in Memory space */
#define PCI_COMMAND_MASTER  0x4	/* Enable bus mastering */

#define  PCI_BASE_ADDRESS_PIO 0x01
#define  PCI_BASE_ADDRESS_MEMORY_TYPE_64 0x04	/* 64 bit address */

typedef struct _PCI_DEVICE
{
	DWORD A;                // BUS DEVICE FUNCTION and ENABLE
	QWORD DEVICE;           // Device struct pointer
	struct _PCI_DEVICE *A0; // Prev node
	struct _PCI_DEVICE *A1; // Next node
} PCI_DEVICE;

extern PCI_DEVICE *ALL_PCI_DEVICE;

void SetupPCI();
void PCIDevice();
DWORD PCIGetClassInterface(DWORD);
void OutputPCIDevice(DWORD);
const char *PCIDeviceName(DWORD);
QWORD PCIEnableMMIO(DWORD, DWORD);

#endif