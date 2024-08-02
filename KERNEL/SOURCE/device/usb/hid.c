#include <device/usb/hid.h>
#include <console/console.h>
#include <declspec.h>
#include <intrinsic.h>
#include <device/xhci.h>
#include <device/keyboard.h>

#define MAX_KBD_EVENT 16

#define HID_REQ_GET_REPORT              0x01
#define HID_REQ_GET_IDLE                0x02
#define HID_REQ_GET_PROTOCOL            0x03
#define HID_REQ_SET_REPORT              0x09
#define HID_REQ_SET_IDLE                0x0A
#define HID_REQ_SET_PROTOCOL            0x0B

#define HID_SET_PROTOCOL_BOOT   0
#define HID_SET_PROTOCOL_REPORT 1

typedef struct _HID_STANDARD_KEYEVENT
{
	BYTE MOD;
	BYTE RSV;
	BYTE KEY[6];
} HID_STANDARD_KEYEVENT;

CODEDECL USB_PIPE *KEYBOARD_PIPE = 0;
CODEDECL HID_STANDARD_KEYEVENT KEYEVENT0;
CODEDECL HID_STANDARD_KEYEVENT KEYEVENT1;
CODEDECL WORD KEYPRESS[16];

void USBKeyboardEvent()
{
	if (KEYBOARD_PIPE)
	{
		// OUTPUTTEXT("USB KEYBOARD EVENT\n");
		// Assume to xHCI pipe
		if (!KEYEVENT1.RSV)
		{
			KEYEVENT1.RSV = 1;
			for (DWORD i = 0; i < 6; i++)
			{
				if (KEYEVENT0.KEY[i])
				{
					BYTE cod = KEYEVENT0.KEY[i];
					BYTE idx = cod >> 4;
					WORD msk = ~(1 << (cod & 0xF));
					int up = 1;
					for (DWORD j = 0; (j < 6) && up; j++)
					{
						if (cod == KEYEVENT1.KEY[j])
						{
							up = 0;
						}
					}
					if (up)
					{
						KEYPRESS[idx] &= msk;
					}
				}
			}
			for (DWORD i = 0; i < 6; i++)
			{
				if (KEYEVENT1.KEY[i])
				{
					BYTE cod = KEYEVENT1.KEY[i];
					BYTE idx = cod >> 4;
					WORD msk = (1 << (cod & 0xF));
					if (!(KEYPRESS[idx] & msk))
					{
						KeyEvent(&KEY_RING, cod);
					}
					KEYPRESS[idx] |= msk;
				}
			}
			memcpy(&KEYEVENT0, &KEYEVENT1, sizeof(HID_STANDARD_KEYEVENT));
		}
		
		XHCI_PIPE *xpipe = (XHCI_PIPE *) KEYBOARD_PIPE;
		if (xpipe->RING.NID == xpipe->RING.EID)
		{
			KEYBOARD_PIPE->CTRL->XFER(KEYBOARD_PIPE, 0, &KEYEVENT1, sizeof(HID_STANDARD_KEYEVENT), 1);
		}
	}
}
DWORD ConfigureKeyboard(USB_COMMON *usbdev, USB_ENDPOINT *epdesc)
{
	if (KEYBOARD_PIPE) return -1; // Only enable first found keyboard

	if (epdesc->MPS < sizeof(HID_STANDARD_KEYEVENT) || epdesc->MPS > MAX_KBD_EVENT)
	{
		OUTPUTTEXT("USB KEYBOARD MAX PACKET SIZE=");
		PRINTRAX(epdesc->MPS, 4);
		LINEFEED();
		return -1;
	}
	
	// SET PROTOCOL
	USB_DEVICE_REQUEST req;
	req.T = USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE;
	req.C = HID_REQ_SET_PROTOCOL;
	req.V = HID_SET_PROTOCOL_BOOT;
	req.I = usbdev->IFC->IN;
	req.L = 0;
	int cc = usbdev->CTRL->XFER(usbdev->PIPE, &req, 0, 0, 0);
	if (cc)
	{
		OUTPUTTEXT("CANNOT SET PROTOCOL ");
		PRINTRAX(cc, 2);
		LINEFEED();
		return cc;
	}

	// SET IDLE 4ms
	req.T = USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE;
	req.C = HID_REQ_SET_IDLE;
	req.V = 1 << 8;
	req.I = 0;
	req.L = 0;
	cc = usbdev->CTRL->XFER(usbdev->PIPE, &req, 0, 0, 0);
	if (cc)
	{
		OUTPUTTEXT("CANNOT SET IDLE ");
		PRINTRAX(cc, 2);
		LINEFEED();
		return cc;
	}

	memset(&KEYEVENT0, 0, sizeof(HID_STANDARD_KEYEVENT));
	memset(&KEYEVENT1, 0, sizeof(HID_STANDARD_KEYEVENT));
	KEYEVENT1.RSV = 1;
	CreateKeyEventRing(&KEY_RING);

	KEYBOARD_PIPE = usbdev->CTRL->CPIP(usbdev, 0, epdesc);
	if (!KEYBOARD_PIPE)
	{
		OUTPUTTEXT("CANNOT CREATE PIPE\n");
		return -1;
	}

	/*
	// Test
	HID_STANDARD_KEYEVENT event;
	while (1)
	{
		memset(&event, 0, sizeof(HID_STANDARD_KEYEVENT));
		event.RSV = 1;
		KEYBOARD_PIPE->CTRL->XFER(KEYBOARD_PIPE, 0, &event, sizeof(HID_STANDARD_KEYEVENT), 0);
		XHCI_PIPE *xpipe = (XHCI_PIPE *) KEYBOARD_PIPE;
		while (xpipe->RING.EID != xpipe->RING.NID)
		{
			XHCIProcessEvent((XHCI_CONTROLLER *) KEYBOARD_PIPE->CTRL);
		}
		OUTPUTTEXT("HID_STANDARD_KEYEVENT");
		BYTE *mem = (BYTE *) &event;
		for (int i = 0; i < 8; i++)
		{
			OUTCHAR(' ');
			PRINTRAX(mem[i], 2);
		}
		LINEFEED();
		__halt();
	}
	*/
	return 0;
}
DWORD ConfigureHID(USB_COMMON *usbdev)
{
	USB_INTERFACE *iface = usbdev->IFC;
	if (iface->IS != USB_INTERFACE_SUBCLASS_BOOT)
	{
		// OUTPUTTEXT("NOT A BOOT DEVICE\n");
		return -1;
	}
	
	USB_ENDPOINT *epdesc = USBSearchEndpoint(usbdev, USB_ENDPOINT_XFER_INT, USB_DIR_IN);
	if (!epdesc)
	{
		OUTPUTTEXT("NO USB HID INTEERRUPT IN\n");
		return -1;
	}
	if (iface->IP == USB_INTERFACE_PROTOCOL_KEYBOARD)
	{
		return ConfigureKeyboard(usbdev, epdesc);
	}
	else if (iface->IP == USB_INTERFACE_PROTOCOL_MOUSE)
	{
		return -1; // ConfigureMouse
	}
	
	return -1;
}