#include <device/pci.h>
#include <types.h>
#include <intrinsic.h>
#include <console/console.h>
#include <declspec.h>
#include <memory/heap.h>
#include <device/xhci.h>
#include <system.h>
#include <device/nvme.h>

CODEDECL const char PCI077A15AD[] = "VMware USB3 xHCI 1.1 Controller";
CODEDECL const char PCI07F015AD[] = "VMware NVMe SSD Controller";
CODEDECL const char PCI501E15B7[] = "Sandisk PC SN735 NVMe SSD (DRAM-less)";
CODEDECL const char PCI91001E95[] = "Solid State Storage Technology CL1-3D256-Q11 NVMe SSD M.2";
CODEDECL const char PCI06ED8086[] = "Intel Corporation Comet Lake USB 3.1 xHCI Host Controller";
CODEDECL const char MSG0800[] = "SETUP PCI\n";
CODEDECL const char MSG0801[] = "PCI(";
CODEDECL const char MSG0802[] = ") ";
CODEDECL const char MSG0803[] = "ERROR:PIO ENABLED\n";
CODEDECL const char MSG0804[] = "ERROR:CAN NOT MAP MEMORY BAR OVER 4G\n";
CODEDECL PCI_DEVICE *ALL_PCI_DEVICE = 0;

void SetupPCI()
{
	OUTPUTTEXT(MSG0800);
	// Detecting pci devices
	PCIDevice();
	// Setup pci devices
	// SetupXHCI();
	// ConfigureNVME();
}
void PCIDevice()
{
	for (DWORD addr = 0x80000000; addr < 0x81000000; addr += 0x100)
	{
		__outdword(PCI_CONFIG_ADDRESS, addr);
		DWORD id = __indword(PCI_CONFIG_DATA);
		WORD vendor = id & 0xFFFF;
		if (id && ~id)
		{
			// Insert head
			PCI_DEVICE *device = (PCI_DEVICE *) HeapAlloc(HEAPK, sizeof(PCI_DEVICE));
			if (device == 0) continue;

			device->CMD = addr;
			device->A1 = ALL_PCI_DEVICE;
			if (ALL_PCI_DEVICE)
			{
				ALL_PCI_DEVICE->A0 = device;
			}
			ALL_PCI_DEVICE = device;

			DWORD class = PCIGetClassInterface(device->CMD);
			switch (class)
			{
				case PCI_CLASS_XHCI:
				{
					SetupXHCIControllerPCI(device);
					break;
				}
				case PCI_CLASS_NVME:
				{
					ConfigureNVME(device);
					break;
				}
			}
		}
	}
}
DWORD PCIGetClassInterface(DWORD addr)
{
	__outdword(PCI_CONFIG_ADDRESS, addr + 0x08);
	return __indword(PCI_CONFIG_DATA) >> 8;
}
void OutputPCIDevice(DWORD addr)
{
	OUTPUTTEXT(MSG0801);
	PRINTRAX((addr >> 16) & 0xFF, 2);
	OUTCHAR('.');
	PRINTRAX((addr >> 11) & 0x1F, 2);
	OUTCHAR('.');
	PRINTRAX((addr >> 8) & 0x7, 1);
	OUTPUTTEXT(MSG0802);
	__outdword(PCI_CONFIG_ADDRESS, addr);
	DWORD id = __indword(PCI_CONFIG_DATA);
	const char *name = PCIDeviceName(id);
	if (name)
	{
		OUTPUTTEXT(name);
	}
	else
	{
		PRINTRAX(id, 8);
	}
	LINEFEED();
}
const char *PCIDeviceName(DWORD id)
{
	WORD vendor = id & 0xFFFF;
	WORD device = id >> 16;
	switch (vendor)
	{
		case 0x15AD:
		{
			switch (device)
			{
				case 0x077A: return PCI077A15AD;
				case 0x07F0: return PCI07F015AD;
			}
			break;
		}
		case 0x15B7:
		{
			switch (device)
			{
				case 0x501E: return PCI501E15B7;
			}
			break;
		}
		case 0x1E95:
		{
			switch (device)
			{
				case 0x9100: return PCI91001E95;
			}
			break;
		}
		case 0x8086:
		{
			switch (device)
			{
				case 0x06ED: return PCI06ED8086;
			}
			break;
		}
	}
	return 0;
}
QWORD PCIEnableMMIO(DWORD addr, DWORD off)
{
	// Read BAR
	__outdword(PCI_CONFIG_ADDRESS, addr + off);
	QWORD bar = __indword(0x0CFC);
	// Check BAR PIO Mode
	if (bar & PCI_BASE_ADDRESS_PIO)
	{
		OUTPUTTEXT(MSG0803);
		return 0;
	}
	// Check BAR size
	if (bar & PCI_BASE_ADDRESS_MEMORY_TYPE_64)
	{
		__outdword(PCI_CONFIG_ADDRESS, addr + off + 4);
		if (__indword(PCI_CONFIG_DATA))
		{
			OUTPUTTEXT(MSG0804);
			return 0;
		}
	}
	bar &= ~0xF;
	bar |= SYSTEM_LINEAR;
	// Read command register
	__outdword(PCI_CONFIG_ADDRESS, addr + PCI_COMMAND);
	WORD cmd = __inword(PCI_CONFIG_DATA);
	// Set Memory Space Enable bit
	cmd |= PCI_COMMAND_MEMORY;
	// Write to command register
	__outdword(PCI_CONFIG_ADDRESS, addr + PCI_COMMAND);
	__outword(PCI_CONFIG_DATA, cmd);
	return bar;
}