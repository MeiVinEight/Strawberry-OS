#include <device/usb.h>
#include <declspec.h>
#include <timer/timer.h>
#include <intrinsic.h>
#include <memory/heap.h>
#include <device/xhci.h>
#include <console/console.h>
#include <device/usb/hub.h>
#include <device/usb/msc.h>

CODEDECL USB_COMMON *ALL_USB_DEVICE = 0;

CODEDECL const DWORD SPEED_TO_CTRL_SIZE[] =
{
	8,   // FULL  SPEED
	8,   // LOW   SPEED
	64,  // HIGH  SPEED
	512, // SUPER SPEED
};

DWORD USBEnumerate(USB_HUB *hub)
{
	for (DWORD port = 0; port < hub->PC; port++)
	{
		if (!hub->OP->DTC(hub, port))
		{
			// No device found.
			goto NEXT_PORT;
		}
		USB_COMMON *common = (USB_COMMON *) HeapAlloc(HEAPK, sizeof(USB_COMMON));
		memset(common, 0, sizeof(USB_COMMON));
		common->CTRL = hub->CTRL;
		common->HUB = hub;
		common->PORT = port;

		// Reset port and determine device speed
		common->SPD = hub->OP->RST(hub, port);
		if (common->SPD < 0)
		{
			goto RESET_FAILED;
		}

		// Set address of port
		if (USBSetAddress(common))
		{
			hub->OP->DCC(hub, port);
			goto RESET_FAILED;
		}
		
		// Configure device
		DWORD count = ConfigureUSB(common);
		if (!count)
		{
			hub->OP->DCC(hub, port);
			HeapFree(common);
		}
		hub->DC += count;
		common->A1 = ALL_USB_DEVICE;
		if (ALL_USB_DEVICE)
		{
			ALL_USB_DEVICE->A0 = common;
		}
		ALL_USB_DEVICE = common;
		

		continue;

		RESET_FAILED:;
		HeapFree(common);

		NEXT_PORT:;
	}
	return hub->DC;
}
DWORD USBSetAddress(USB_COMMON *device)
{
	USB_CONTROLLER *controller = device->CTRL;
	if (controller->MA >= USB_MAXADDR) return 1;

	__halt();
	__halt();

	// Create a pipe for the default address
	USB_ENDPOINT epdesc;
	memset(&epdesc, 0, sizeof(USB_ENDPOINT));
	epdesc.MPS = SPEED_TO_CTRL_SIZE[device->SPD];
	epdesc.AT = USB_ENDPOINT_XFER_CONTROL;
	device->PIPE = controller->CPIP(device, 0, &epdesc);
	if (!device->PIPE)
	{
		return -1;
	}

	// Send SET_ADDRESS command.
	USB_DEVICE_REQUEST req;
	req.T = USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
	req.C = USB_REQ_SET_ADDRESS;
	req.V = controller->MA = 1;
	req.I = 0;
	req.L = 0;
	DWORD cc = controller->XFER(device->PIPE, &req, 0, 0);
	if (cc)
	{
		OUTPUTTEXT("CANNOT SET ADDRESS ");
		PRINTRAX(cc, 2);
		LINEFEED();
		// Free pipe
		return -1;
	}

	__halt();
	controller->MA++;
	device->PIPE = controller->CPIP(device, device->PIPE, &epdesc);
	if (!device->PIPE)
	{
		return -1;
	}
	return 0;
}
DWORD ConfigureUSB(USB_COMMON *common)
{
	USB_CONTROLLER *controller = common->CTRL;
	// Set the max packet size for endpoint 0 of this device
	// Get first 8 byte of device desc
	USB_DEVICE devinfo;
	USB_DEVICE_REQUEST req;
	req.T = USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
	req.C = USB_REQ_GET_DESCRIPTOR;
	req.V = USB_DT_DEVICE << 8;
	req.I = 0;
	req.L = 8;
	DWORD cc = controller->XFER(common->PIPE, &req, &devinfo, 0);
	if (cc)
	{
		OUTPUTTEXT("TRANSFER FAILED ");
		PRINTRAX(cc, 2);
		LINEFEED();
		return 0;
	}
	OUTPUTTEXT("USB DEVICE DT=");
	PRINTRAX(devinfo.DT, 2);
	OUTPUTTEXT(" VER=");
	PRINTRAX(devinfo.VER, 4);
	OUTPUTTEXT(" DC=");
	PRINTRAX(devinfo.DC, 2);
	OUTPUTTEXT(" DS=");
	PRINTRAX(devinfo.DS, 2);
	OUTPUTTEXT(" DP=");
	PRINTRAX(devinfo.DP, 2);
	OUTPUTTEXT(" MPS=");
	PRINTRAX(devinfo.MPS, 2);
	LINEFEED();

	WORD maxpacket = devinfo.MPS;
	if (devinfo.VER >= 0x0300) maxpacket = 1 << devinfo.MPS;
	if (maxpacket < 8) return 0;
	USB_ENDPOINT epdesc = {
		.MPS = maxpacket,
		.AT = USB_ENDPOINT_XFER_CONTROL,
	};
	common->PIPE = controller->CPIP(common, common->PIPE, &epdesc);

	// Get device config
	USB_CONFIG *config = 0;
	{
		USB_CONFIG cfg;
		memset(&cfg, 0, sizeof(USB_CONFIG));
		req.T = USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
		req.C = USB_REQ_GET_DESCRIPTOR;
		req.V = USB_DT_CONFIG << 8;
		req.I = 0;
		req.L = sizeof(USB_CONFIG);
		if (controller->XFER(common->PIPE, &req, &cfg, 0))
		{
			goto LABEL001;
		}

		config = HeapAlloc(HEAPK, cfg.TL);
		memset(config, 0, cfg.TL);
		req.L = cfg.TL;
		if (controller->XFER(common->PIPE, &req, config, 0) || config->TL != cfg.TL)
		{
			HeapFree(config);
			config = 0;
		}
	}
	LABEL001:;
	if (!config)
	{
		OUTPUTTEXT("CANNOT GET CONFIG\n");
		return 0;
	}


	QWORD ic = config->IC;
	QWORD iface = ((QWORD) config) + 9;
	QWORD cfend = ((QWORD) config) + config->TL;

	while (1)
	{
		USB_INTERFACE *interface = (USB_INTERFACE *) iface;
		if ((!ic) || ((iface + interface->L) > cfend)) break; // goto FAILED;

		if (interface->DT == USB_DT_INTERFACE)
		{
			ic--;
			OUTPUTTEXT("USB INTERFACE L=");
			PRINTRAX(interface->L, 2);
			OUTPUTTEXT(" DT=");
			PRINTRAX(interface->DT, 2);
			OUTPUTTEXT(" IN=");
			PRINTRAX(interface->IN, 2);
			OUTPUTTEXT(" AS=");
			PRINTRAX(interface->AS, 2);
			OUTPUTTEXT(" EC=");
			PRINTRAX(interface->EC, 2);
			OUTPUTTEXT(" IC=");
			PRINTRAX(interface->IC, 2);
			OUTPUTTEXT(" IS=");
			PRINTRAX(interface->IS, 2);
			OUTPUTTEXT(" IP=");
			PRINTRAX(interface->IP, 2);
			OUTPUTTEXT(" II=");
			PRINTRAX(interface->II, 2);
			LINEFEED();
		}
		iface += interface->L;
	}


	req.T = USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
	req.C = USB_REQ_SET_CONFIGURATION;
	req.V = config->CV;
	req.I = 0;
	req.L = 0;
	cc = controller->XFER(common->PIPE, &req, 0, 0);
	if (cc)
	{
		OUTPUTTEXT("CANNOT SET CONFIGURATION\n");
		goto CONFIG_FAIL;
	}

	USB_INTERFACE *interface = (USB_INTERFACE *) (((QWORD) config) + 9);
	common->CFG = config;
	common->IFC = interface;
	if (interface->IC == USB_CLASS_HUB)
	{
		cc = ConfigureHUB(common);
	}
	else if (interface->IC == USB_CLASS_MASS_STORAGE)
	{
		if (interface->IP == US_PR_BULK)
		{
			cc = ConfigureMSC(common, interface);
		}
	}
	if (cc)
	{
		OUTPUTTEXT("CANNOT CONFIGURE DEVICE\n");
		goto CONFIG_FAIL;
	}

	return 1;

	CONFIG_FAIL:;
	HeapFree(config);
	common->CFG = 0;
	common->IFC = 0;
	return 0;
}
void USBD2P(USB_PIPE *pipe, USB_COMMON *usbdev, USB_ENDPOINT *epdesc)
{
	pipe->CTRL = usbdev->CTRL;
	pipe->MPS = epdesc->MPS;
}
USB_ENDPOINT *USBSearchEndpoint(USB_COMMON *usbdev, int type, int dir)
{
	USB_CONFIG *config = usbdev->CFG;
	USB_INTERFACE *iface = usbdev->IFC;
	QWORD imax = ((QWORD) config) + config->TL - ((QWORD) iface);
	USB_ENDPOINT *epdesc = (USB_ENDPOINT *) (((QWORD) iface) + 9);
	while (1)
	{
		if ((((QWORD) epdesc) >= ((QWORD) iface) + imax) || (epdesc->DT == USB_DT_INTERFACE)) return 0;
		DWORD edir = epdesc->EA & USB_ENDPOINT_DIR;
		DWORD etyp = epdesc->AT & USB_ENDPOINT_XFER_TYPE;
		if ((epdesc->DT == USB_DT_ENDPOINT) && (edir == dir) && (etyp == type)) return epdesc;
		epdesc = (USB_ENDPOINT *) (((QWORD) epdesc) + epdesc->L);
	}
	return 0;
}
