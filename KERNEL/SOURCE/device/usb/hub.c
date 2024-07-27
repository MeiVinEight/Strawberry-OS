#include <device/usb/hub.h>
#include <console/console.h>
#include <intrinsic.h>
#include <declspec.h>

CODEDECL USB_HUB_OPERATION HUB_OPERATION;

int ConfigureHUB(USB_COMMON *common)
{
	if (!HUB_OPERATION.DTC)
	{
		HUB_OPERATION.DTC = USBHUBDetect;
		HUB_OPERATION.RST = USBHUBReset;
		HUB_OPERATION.DCC = USBHUBDisconnect;
	}

	OUTPUTTEXT("CONFIGURE HUB\n");
	USB_HUB_DESCRIPTOR desc;
	USB_DEVICE_REQUEST req;
	req.T = USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_DEVICE;
	req.C = USB_REQ_GET_DESCRIPTOR;
	if (common->SPD == USB_SUPERSPEED)
	{
		req.V = USB_DT_HUB3 << 8;
	}
	else
	{
		req.V = USB_DT_HUB << 8;
	}
	req.I = 0;
	req.L = sizeof(USB_HUB_DESCRIPTOR);
	int cc = common->CTRL->XFER(common->PIPE, &req, &desc, 0);
	if (cc)
	{
		OUTPUTTEXT("CANNOT GET HUB DESCRIPTOR\n");
		return cc;
	}

	USB_HUB hub;
	memset(&hub, 0, sizeof(USB_HUB));
	hub.DEVICE = common;
	hub.CTRL = common->CTRL;
	hub.PC = desc.NP;
	hub.OP = &HUB_OPERATION;


	return 0;
}
DWORD USBHUBDetect(USB_HUB *hub, DWORD port)
{
	USB_PORT_STATUS sts;
	USB_DEVICE_REQUEST req;
	req.T = USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_OTHER;
	req.C = USB_REQ_GET_STATUS;
	req.V = 0;
	req.I = port + 1;
	req.L = sizeof(USB_PORT_STATUS);
	int cc = hub->DEVICE->CTRL->XFER(hub->DEVICE->PIPE, &req, &sts, 0);
	if (cc)
	{
		OUTPUTTEXT("FAILURT ON HUB PORT ");
		PRINTRAX(port, 4);
		OUTPUTTEXT(" DETECT\n");
		return -1;
	}
	return (sts.PS & USB_PORT_STAT_CONNECTION);
}
DWORD USBHUBReset(USB_HUB *hub, DWORD port)
{
	return 0;
}
DWORD USBHUBDisconnect(USB_HUB *hub, DWORD port)
{
	return 0;
}