#include <device/xhci.h>
#include <declspec.h>
#include <console/console.h>
#include <device/pci.h>
#include <intrinsic.h>
#include <memory/heap.h>
#include <system.h>
#include <timer/timer.h>
#include <memory/page.h>
#include <device/usb.h>
#include <interrupt/interrupt.h>

#define PCI_CLASS_XHCI 0x0C0330

CODEDECL const char MSG0900[] = "SETUP XHCI\n";
CODEDECL const char MSG0901[] = "ERROR:XHCI PAGE SIZE ";
CODEDECL const char MSG0902[] = "ERROR:XHCI WAIT TIMEOUT\n";
CODEDECL const char MSG0903[] = "UNKNOWN EVENT TYPE ";
CODEDECL const DWORD SPEED_XHCI[16] =
{
	-1,
	USB_FULLSPEED,
	USB_LOWSPEED,
	USB_HIGHSPEED,
	USB_SUPERSPEED,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};
CODEDECL USB_HUB_OPERATION XHCI_OPERARTION;

void InterruptXHCI(INTERRUPT_STACK *stack)
{
	XHCI_CONTROLLER *controller = (XHCI_CONTROLLER *) USB_CTRL;
	while (XHCIProcessEvent(controller));
	/*
	* EHB bit is a W1C bit, Write 1 to clear
	*/
	controller->RR->IR[0].ERD = controller->RR->IR[0].ERD & (~0x7ULL);
}
void SetupXHCI()
{
	OUTPUTTEXT(MSG0900);
	XHCI_OPERARTION.DTC = XHCIHUBDetect;
	XHCI_OPERARTION.RST = XHCIHUBReset;
	XHCI_OPERARTION.DCC = XHCIHUBDisconnect;
	// Foreach device list
	PCI_DEVICE *dvc = ALL_PCI_DEVICE;
	while (dvc)
	{
		if (PCIGetClassInterface(dvc->CMD) == PCI_CLASS_XHCI)
		{
			SetupXHCIControllerPCI(dvc);
		}
		dvc = dvc->A1;
	}
}
void SetupXHCIControllerPCI(PCI_DEVICE *dvc)
{
	OutputPCIDevice(dvc->CMD);
	// Enable this pci device mmio
	QWORD bar = PCIEnableMMIO(dvc->CMD, PCI_BASE_ADDRESS_0);
	if (!bar)
	{
		return;
	}
	// Enable busmaster
	// Read command register and set MASTER bit
	__outdword(PCI_CONFIG_ADDRESS, dvc->CMD + PCI_COMMAND);
	WORD val = __inword(PCI_CONFIG_DATA) | PCI_COMMAND_MASTER;
	// Write to command register
	__outdword(PCI_CONFIG_ADDRESS, dvc->CMD + PCI_COMMAND);
	__outword(PCI_CONFIG_DATA, val);

	__outdword(PCI_CONFIG_ADDRESS, dvc->CMD + PCI_BASE_CAPABILITY);
	WORD cp = __indword(PCI_CONFIG_DATA) & 0xFF;
	while (cp)
	{
		// PRINTRAX(cp, 4);
		// OUTCHAR(' ');
		__outdword(PCI_CONFIG_ADDRESS, dvc->CMD + cp);
		DWORD capa = __indword(PCI_CONFIG_DATA);
		WORD capid = capa & 0xFF;
		// PRINTRAX(capid, 2);
		// LINEFEED();
		if (capid == 5)
		{
			// Interrupt Vector may not exceed 0x2F ?
			BYTE xhciIntVec = 0x21;
			if (capa & 0x00800000)
			{
				// MSI Capability 64
				capa |= (1 << 16); // MSI Enable
				__outdword(PCI_CONFIG_ADDRESS, dvc->CMD + cp);
				__outdword(PCI_CONFIG_DATA, capa);
				__outdword(PCI_CONFIG_ADDRESS, dvc->CMD + cp + 4); // MSI Message Address low 32
				__outdword(PCI_CONFIG_DATA, 0xFEE00000);
				__outdword(PCI_CONFIG_ADDRESS, dvc->CMD + cp + 8); // MSI Message Address high 32
				__outdword(PCI_CONFIG_DATA, 0);
				__outdword(PCI_CONFIG_ADDRESS, dvc->CMD + cp + 12); // MSI Message Data
				__outword(PCI_CONFIG_DATA, xhciIntVec);
			}
			else
			{
				// MSI Capability 32
				capa |= (1 << 16); // MSI Enable
				__outdword(PCI_CONFIG_ADDRESS, dvc->CMD + cp);
				__outdword(PCI_CONFIG_DATA, capa);
				__outdword(PCI_CONFIG_ADDRESS, dvc->CMD + cp + 4); // MSI Message Address 32
				__outdword(PCI_CONFIG_DATA, 0xFEE00000);
				__outdword(PCI_CONFIG_ADDRESS, dvc->CMD + cp + 8); // MSI Message Address 32
				__outword(PCI_CONFIG_DATA, xhciIntVec);
			}
			// Interrupt Vector may not exceed 0x2F ?
			register_interrupt(xhciIntVec, InterruptXHCI);
		}
		else if (capid == 0x11)
		{
			// Disable MSI-X
			capa &= ~(1 << 31); // MSI-X Enable
			__outdword(PCI_CONFIG_ADDRESS, dvc->CMD + cp);
			__outdword(PCI_CONFIG_DATA, capa);
		}
		cp = (capa >> 8) & 0xFF;
	}

	XHCI_CONTROLLER *controller = SetupXHCIController(bar);
	if (!controller)
	{
		return;
	}
	dvc->DEVICE = (QWORD) controller;
	USB_CTRL = (USB_CONTROLLER *) controller;
	if (ConfigureXHCI(controller))
	{
		HeapFree(controller);
		dvc->DEVICE = 0;
	}
}
XHCI_CONTROLLER *SetupXHCIController(QWORD bar)
{
	XHCI_CONTROLLER *controller = (XHCI_CONTROLLER *) HeapAlloc(HEAPK, sizeof(XHCI_CONTROLLER));
	memset(controller, 0, sizeof(XHCI_CONTROLLER));
	controller->USB.TYPE = USB_TYPE_XHCI;
	controller->CR = (XHCI_CAPABILITY *)  (bar);
	controller->OR = (XHCI_OPERATIONAL *) (bar + controller->CR->CL);
	controller->PR = (XHCI_PORT *)        (bar + controller->CR->CL + 0x400);
	controller->RR = (XHCI_RUNTIME *)     (bar + controller->CR->RRSO);
	controller->DR = (DWORD *)            (bar + controller->CR->DBO);

	DWORD sp1 = controller->CR->SP1;
	DWORD cp1 = controller->CR->CP1;
	controller->SN = (sp1 >>  0) & 0xFF;
	controller->IN = (sp1 >>  8) & 0x7FF;
	controller->PN = (sp1 >> 24) & 0xFF;
	controller->XEC = ((cp1 >> 16) & 0xFFFF) << 2;
	controller->CSZ = ((cp1 >>  2) & 0x0001);

	if (controller->XEC)
	{
		DWORD off = ~0;
		QWORD addr = bar + controller->XEC;
		while (off)
		{
			XHCI_XCAPABILITY *xcap = (XHCI_XCAPABILITY *) addr;
			DWORD cap = xcap->CAP;
			if ((cap & 0xFF) == 0x02)
			{
				QWORD name = xcap->DATA[0];
				DWORD ports = xcap->DATA[1];
				BYTE major = (cap >> 24) & 0xFF;
				BYTE minor = (cap >> 16) & 0xFF;
				BYTE count = (ports >> 8) & 0xFF;
				BYTE start = (ports >> 0) & 0xFF;
				QWORD txt = 0x2049434858; // XHCI
				OUTPUTTEXT((char *) &txt);
				OUTPUTTEXT((char *) &name);
				OUTCHAR(' ');
				PRINTRAX(major, 1);
				OUTCHAR('.');
				PRINTRAX(minor, 2);
				OUTCHAR(' ');
				PRINTRAX(count, 2);
				txt = 0x2B2820;// (+
				OUTPUTTEXT((char *) &txt);
				PRINTRAX(start - 1, 2);
				txt = 0x2029; // )
				OUTPUTTEXT((char *) &txt);
				PRINTRAX(ports >> 16, 4);
				LINEFEED();
			}
			else
			{
				/*
				QWORD txt = 0x2049434858; // XHCI
				OUTPUTTEXT((char *) &txt);
				txt = 0x2050414358; // XCAP
				OUTPUTTEXT((char *) &txt);
				PRINTRAX(cap & 0xFF, 2);
				txt = 0x204020; // @ 
				OUTPUTTEXT((char *) &txt);
				PRINTRAX(addr, 16);
				LINEFEED();
				*/
			}
			off = (cap >> 8) & 0xFF;
			addr += (QWORD) off << 2;
		}
	}

	DWORD pageSize = controller->OR->PS;
	if (pageSize != 1)
	{
		OUTPUTTEXT(MSG0901);
		PRINTRAX(pageSize, 8);
		LINEFEED();
		HeapFree(controller);
		return 0;
	}

	controller->USB.CPIP = XHCICreatePipe;
	controller->USB.XFER = XHCITransfer;

	return controller;
}
DWORD ConfigureXHCI(XHCI_CONTROLLER *controller)
{
	QWORD physicalAddress = 0;
	QWORD pageCount = 1;
	AllocatePhysicalMemory(&physicalAddress, PAGE4_4K, &pageCount);
	physicalAddress |= SYSTEM_LINEAR;
	memset((void *) physicalAddress, 0, 4096);
	controller->DVC = (QWORD *)             (physicalAddress + 0x000);
	controller->SEG = (XHCI_RING_SEGMENT *) (physicalAddress + 0x800);
	XHCICreateTransferRing(&controller->CMD);
	XHCICreateTransferRing(&controller->EVT);

	// Reset controller
	/*
	DWORD reg = controller->OR->CMD;
	reg &= ~XHCI_CMD_INTE;
	reg &= ~XHCI_CMD_HSEE;
	reg &= ~XHCI_CMD_EWE;
	reg &= ~XHCI_CMD_RS;
	controller->OR->CMD = reg;
	while (!(controller->OR->STS & XHCI_STS_HCH)) __halt();
	controller->OR->CMD |= XHCI_CMD_HCRST;
	while (controller->OR->CMD & XHCI_CMD_HCRST) __halt();
	while (controller->OR->STS & XHCI_STS_CNR) __halt();
	*/
	DWORD reg = controller->OR->CMD;
	if (reg & XHCI_CMD_RS)
	{
		reg &= ~XHCI_CMD_RS;
		controller->OR->CMD = reg;
		while (!(controller->OR->STS & XHCI_STS_HCH)) __halt();
	}
	controller->OR->CMD = XHCI_CMD_HCRST;
	while (controller->OR->CMD & XHCI_CMD_HCRST) __halt();
	while (controller->OR->STS & XHCI_STS_CNR) __halt();

	controller->OR->CFG = controller->SN;
	controller->CMD.CCS = 1;
	controller->OR->CRC = physical_mapping((QWORD) controller->CMD.RING) | 1;
	controller->OR->DCA = physical_mapping((QWORD) controller->DVC);

	controller->SEG->A = physical_mapping((QWORD) controller->EVT.RING);
	controller->SEG->S = XHCI_RING_ITEMS;

	controller->EVT.CCS = 1;
	controller->RR->IR[0].IMOD = 0;
	controller->RR->IR[0].IMAN |= 2; // Interrupt Enable
	controller->RR->IR[0].TS = 1;
	controller->RR->IR[0].ERS = physical_mapping((QWORD) controller->SEG);
	controller->RR->IR[0].ERD = physical_mapping((QWORD) controller->EVT.RING);

	reg = controller->CR->SP2;
	DWORD spb = (reg >> 21 & 0x1F) << 5 | reg >> 27;
	if (spb)
	{
		OUTPUTTEXT("CREATE SCRATCHPAD ");
		PRINTRAX(spb, 3);
		LINEFEED();
		QWORD pageAddress = 0;
		QWORD pageCount = 1;
		QWORD *spba = 0;
		AllocatePhysicalMemory(&pageAddress, PAGE4_4K, &pageCount);
		spba = (QWORD *) (pageAddress | SYSTEM_LINEAR);
		memset(spba, 0, 4096);
		for (DWORD i = 0; i < spb; i++)
		{
			pageAddress = 0;
			pageCount = 1;
			AllocatePhysicalMemory(&pageAddress, PAGE4_4K, &pageCount);
			memset((void *) (pageAddress | SYSTEM_LINEAR), 0, 4096);
			spba[i] = pageAddress;
		}
		controller->DVC[0] = physical_mapping((QWORD) spba);
	}

	controller->OR->CMD |= XHCI_CMD_INTE;
	controller->OR->CMD |= XHCI_CMD_RS;
	
	// Find devices
	__halt();
	__halt();
	USB_HUB *hub = &controller->USB.RH;
	hub->CTRL = &controller->USB;
	hub->PC = controller->PN;
	hub->OP = &XHCI_OPERARTION;
	int count = USBEnumerate(hub);
	// xhci_free_pipes
	if (count) return 0; // Success

	// No devices found - shutdown and free controller.
	OUTPUTTEXT("XHCI NO DEVICE FOUND\n");
	controller->OR->CMD &= ~XHCI_CMD_RS;
	while (!(controller->OR->STS & XHCI_STS_HCH)) __halt();

	FAILED:;
	FreePhysicalMemory(physicalAddress, PAGE4_4K, 1);
	return 1;
}
void XHCIQueueTRB(XHCI_TRANSFER_RING *ring, XHCI_TRANSFER_BLOCK *block)
{
	if (ring->NID >= XHCI_RING_ITEMS - 1)
	{
		XHCI_TRB_LINK trb;
		memset(&trb, 0, sizeof(XHCI_TRB_LINK));
		trb.RSP = physical_mapping((QWORD) ring->RING) >> 4;
		trb.TYPE = TRB_LINK;
		trb.TC = 1;
		XHCICopyTRB(ring, &trb.TRB);
		ring->NID = 0;
		ring->CCS ^= 1;
	}
	XHCICopyTRB(ring, block);
	ring->NID++;
}
void XHCICopyTRB(XHCI_TRANSFER_RING *ring, XHCI_TRANSFER_BLOCK *element)
{
	XHCI_TRANSFER_BLOCK *dst = ring->RING + ring->NID;
	dst->DATA[0] = element->DATA[0];
	dst->DATA[1] = element->DATA[1];
	dst->DATA[2] = element->DATA[2];
	dst->DATA[3] = element->DATA[3] | ring->CCS;
}
void XHCIDoorbell(XHCI_CONTROLLER *controller, DWORD slot, DWORD value)
{
	controller->DR[slot] = value;
}
DWORD XHCIProcessEvent(XHCI_CONTROLLER *controller)
{
	// Check for event
	XHCI_TRANSFER_RING *event = &controller->EVT;
	DWORD nid = event->NID;
	XHCI_TRANSFER_BLOCK *trb = event->RING + nid;
	if (trb->TYPE == 0) return 0;
	if ((trb->C != event->CCS)) return 0;

	// Process event
	DWORD eventType = trb->TYPE;
	switch (eventType)
	{
		case TRB_TRANSFER:
		case TRB_COMMAND_COMPLETE:
		{
			XHCI_TRB_COMMAND_COMPLETION *cc = (XHCI_TRB_COMMAND_COMPLETION *) trb;
			QWORD ctp = cc->CTP | SYSTEM_LINEAR;
			XHCI_TRANSFER_BLOCK *firstTRB = (XHCI_TRANSFER_BLOCK *) ((ctp >> 12) << 12);
			XHCI_TRB_TRANSFER_RING *ringTRB = (XHCI_TRB_TRANSFER_RING *) (firstTRB + XHCI_RING_ITEMS);
			XHCI_TRANSFER_RING *ring = (XHCI_TRANSFER_RING *) (ringTRB->RING | SYSTEM_LINEAR);
			DWORD eid = ((cc->CTP & 0xFF0) >> 4) + 1;
			memcpy(&ring->EVT, trb, sizeof(XHCI_TRANSFER_BLOCK));
			ring->EID = eid;

			break;
		}
		case TRB_PORT_STATUS_CHANGE:
		{
			DWORD port = ((trb->DATA[0] >> 24) & 0xFF) - 1;
			DWORD psc = controller->PR[port].PSC;
			// DWORD pcl = (((psc & ~(XHCI_PORTSC_PED | XHCI_PORTSC_PR)) & ~(0x1E0)) | (0x20));
			DWORD pcl = (((psc & ~(XHCI_PORTSC_PED | XHCI_PORTSC_PR)) & ~(0xF << 5)) | (1 << 5));
			controller->PR[port].PSC = pcl;
			break;
		}
		default:
		{
			OUTPUTTEXT(MSG0903);
			PRINTRAX(eventType, 2);
			LINEFEED();
		}
	}
	memset(trb, 0, sizeof(XHCI_TRANSFER_BLOCK));

	// Move ring index, notify xhci
	nid++;
	if (nid == XHCI_RING_ITEMS)
	{
		nid = 0;
		event->CCS ^= 1;
	}
	event->NID = nid;
	controller->RR->IR[0].ERD = physical_mapping((QWORD) (event->RING + event->NID));
	return 1;
}
DWORD XHCIWaitCompletion(XHCI_CONTROLLER *controller, XHCI_TRANSFER_RING *ring)
{
	while (ring->EID != ring->NID)
	{
		// XHCIProcessEvent(controller); // Use xhci interrupt
		__halt();
	}
	return ring->EVT.DATA[2] >> 24;
}
DWORD XHCICommand(XHCI_CONTROLLER *controller, XHCI_TRANSFER_BLOCK *trb)
{
	XHCIQueueTRB(&controller->CMD, trb);
	controller->DR[0] = 0;
	return XHCIWaitCompletion(controller, &controller->CMD);
}
DWORD XHCIHUBDetect(USB_HUB *hub, DWORD port)
{
	XHCI_CONTROLLER *controller = (XHCI_CONTROLLER *) hub->CTRL;
	return (controller->PR[port].PSC & XHCI_PORTSC_CCS);
}
DWORD XHCIHUBReset(USB_HUB *hub, DWORD port)
{
	XHCI_CONTROLLER *controller = (XHCI_CONTROLLER *) hub->CTRL;
	if (!(controller->PR[port].PSC & XHCI_PORTSC_CCS)) return -1;

	switch (((controller->PR[port].PSC >> 5) & 0xF))
	{
		case PLS_U0:
		{
			// A USB 3 port - controller automatically performs reset
			break;
		}
		case PLS_POLLING:
		{
			// A USB 2 port - perform device reset
			controller->PR[port].PSC |= XHCI_PORTSC_PR;
			break;
		}
		default: return -1;
	}

	// Wait for device to complete reset and be enabled
	__halt();
	__halt();
	__halt();
	while (1)
	{
		DWORD psc = controller->PR[port].PSC;
		if (!(psc & XHCI_PORTSC_CCS))
		{
			// Device disconnected during reset
			return -1;
		}
		if (psc & XHCI_PORTSC_PED)
		{
			// Reset complete
			break;
		}
		__halt();
	}

	QWORD txt = 0x2049434858; // XHCI 
	OUTPUTTEXT((char *) &txt);
	txt = 0x20425355; // USB 
	OUTPUTTEXT((char *) &txt);
	PRINTRAX(port, 2);
	txt = 0x2044505320;
	OUTPUTTEXT((char *) &txt);
	PRINTRAX((controller->PR[port].PSC >> 10) & 0xF, 1);
	if (controller->PR[port].PSC & XHCI_PORTSC_PED)
	{
		txt = 0x4E4520; // EN
		OUTPUTTEXT((char *) &txt);
	}
	if (controller->PR[port].PSC & XHCI_PORTSC_PP)
	{
		txt = 0x575020; // PW
		OUTPUTTEXT((char *) &txt);
	}
	LINEFEED();

	return SPEED_XHCI[(controller->PR[port].PSC >> 10) & 0xF];
}
DWORD XHCIHUBDisconnect(USB_HUB *hub, DWORD port)
{
	// XXX - should turn the port power off.
	return 0;
}
QWORD XHCICreateInputContext(USB_COMMON *usbdev, DWORD maxepid)
{
	XHCI_CONTROLLER *controller = (XHCI_CONTROLLER *) usbdev->CTRL;
	QWORD size = (sizeof(XHCI_INPUT_CONTROL_CONTEXT) << controller->CSZ) * 33;
	XHCI_INPUT_CONTROL_CONTEXT *icctx = 0;
	QWORD pageCount = 1;
	AllocatePhysicalMemory((QWORD *) &icctx, PAGE4_4K, &pageCount);
	icctx = (XHCI_INPUT_CONTROL_CONTEXT *) (((QWORD) icctx) | SYSTEM_LINEAR);
	memset(icctx, 0, size);

	XHCI_SLOT_CONTEXT *sctx = (XHCI_SLOT_CONTEXT *) (icctx + (1ULL << controller->CSZ));
	sctx->CE = maxepid;
	sctx->SPD = usbdev->SPD + 1;

	// Set high-speed hub flags.
	if (usbdev->HUB->DEVICE)
	{
		OUTPUTTEXT("SETUP HUB\n");
		USB_COMMON *hubdev = usbdev->HUB->DEVICE;
		if (usbdev->SPD == USB_LOWSPEED || usbdev->SPD == USB_FULLSPEED)
		{
			XHCI_PIPE *hpipe = (XHCI_PIPE *) hubdev->PIPE;
			if (hubdev->SPD == USB_HIGHSPEED)
			{
				sctx->TTID = hpipe->SID;
				sctx->TTP = usbdev->PORT + 1;
			}
			else
			{
				XHCI_SLOT_CONTEXT *hsctx = (XHCI_SLOT_CONTEXT *) (controller->DVC[hpipe->SID] | SYSTEM_LINEAR);
				sctx->TTID = hsctx->TTID;
				sctx->TTP = hsctx->TTP;
				sctx->TTT = hsctx->TTT;
				sctx->IT = hsctx->IT;
			}
		}
		DWORD route = 0;
		while (usbdev->HUB->DEVICE)
		{
			route <<= 4;
			route |= (usbdev->PORT + 1) & 0xF;
			usbdev = usbdev->HUB->DEVICE;
		}
		sctx->RS = route;
	}

	sctx->RHPN = usbdev->PORT + 1;
	return (QWORD) icctx;
}
USB_PIPE *XHCICreatePipe(USB_COMMON *common, USB_PIPE *upipe, USB_ENDPOINT *epdesc)
{
	if (!epdesc)
	{
		// Free
		if (upipe)
		{
			XHCI_PIPE *xpipe = (XHCI_PIPE *) upipe;
			FreePhysicalMemory((QWORD) xpipe->RING.RING, PAGE4_4K, 1);
			HeapFree(xpipe);
		}
		return 0;
	}
	if (!upipe)
	{
		XHCI_CONTROLLER *controller = (XHCI_CONTROLLER *) common->CTRL;
		BYTE eptype = epdesc->AT & USB_ENDPOINT_XFER_TYPE;
		DWORD epid = 1;
		if (epdesc->EA)
		{
			epid = (epdesc->EA & 0x0f) << 1;
			epid += (epdesc->EA & USB_DIR_IN) ? 1 : 0;
		}

		XHCI_PIPE *pipe = HeapAlloc(HEAPK, sizeof(XHCI_PIPE));
		memset(pipe, 0, sizeof(XHCI_PIPE));
		USBD2P(&pipe->USB, common, epdesc);
		pipe->EPID = epid;
		XHCICreateTransferRing(&pipe->RING);
		pipe->RING.CCS = 1;

		// Allocate input context and initialize endpoint info
		XHCI_INPUT_CONTROL_CONTEXT *icctx = (XHCI_INPUT_CONTROL_CONTEXT *) XHCICreateInputContext(common, epid);
		icctx->ADD = 1 | (1 << epid);
		XHCI_ENDPOINT_CONTEXT *epctx = (XHCI_ENDPOINT_CONTEXT *) (icctx + ((pipe->EPID + 1ULL) << controller->CSZ));

		if (eptype == USB_ENDPOINT_XFER_INT)
		{
			OUTPUTTEXT("USB GET PERIOD\n");
			// usb_get_period
			DWORD period = epdesc->ITV;
			if (common->SPD != USB_HIGHSPEED)
			{
				if (period)
				{
					// BSR period, x
					WORD x = period;
					period = 0;
					while (!(x & (1 << 15)))
					{
						x <<= 1;
						period++;
					}
				}
			}
			else
			{
				period = (period <= 4) ? 0 : (period - 4);
			}
			epctx->ITV = period + 3;
		}
		epctx->EPT = eptype;
		
		if (eptype == USB_ENDPOINT_XFER_CONTROL)
		{
			epctx->EPT = EP_CONTROL;
		}
		
		epctx->MPSZ = pipe->USB.MPS;
		epctx->TRDP = physical_mapping((QWORD) pipe->RING.RING) >> 4;
		epctx->DCS = 1;
		epctx->ATL = pipe->USB.MPS;

		if (pipe->EPID == 1)
		{
			if (common->HUB->DEVICE)
			{
				OUTPUTTEXT("CONFIGURE HUB\n");
				// Make sure parent hub is configured.
				USB_HUB *hub = common->HUB;
				XHCI_SLOT_CONTEXT *hubsctx = (XHCI_SLOT_CONTEXT *) (controller->DVC[pipe->SID] | SYSTEM_LINEAR);
				if (hubsctx->SS == 3) // Configured
				{
					// Already configured
					goto HUB_CONFIG_OVER;
				}
				XHCI_INPUT_CONTROL_CONTEXT *icctx = (XHCI_INPUT_CONTROL_CONTEXT *) XHCICreateInputContext(hub->DEVICE, 1);
				icctx->ADD = 1;
				XHCI_SLOT_CONTEXT *sctx = (XHCI_SLOT_CONTEXT *) (icctx + (1ULL << controller->CSZ));
				sctx->HUB = 1;
				sctx->PN = hub->PC;

				XHCI_TRB_CONFIGURE_ENDPOINT trb;
				memset(&trb, 0, sizeof(XHCI_TRB_CONFIGURE_ENDPOINT));
				trb.ICP = physical_mapping((QWORD) icctx);
				trb.TYPE = TRB_CONFIGURE_ENDPOINT;
				trb.SID = pipe->SID;
				DWORD cc;
				if (cc = XHCICommand(controller, &trb.TRB) != 1)
				{
					OUTPUTTEXT("CONFIGURE HUB FAILED ");
					PRINTRAX(cc, 2);
					LINEFEED();
					goto FAILED;
				}

				HUB_CONFIG_OVER:;
			}
			
			// Enable slot
			XHCI_TRB_ENABLE_SLOT trb00;
			memset(&trb00, 0, sizeof(XHCI_TRB_ENABLE_SLOT));
			trb00.TYPE = TRB_ENABLE_SLOT;
			DWORD cc = XHCICommand(controller, &trb00.TRB);
			if (cc != 1)
			{
				OUTPUTTEXT("ENABLE SLOT FAILED ");
				PRINTRAX(cc, 2);
				LINEFEED();
				goto FAILED;
			}
			DWORD slot = controller->CMD.EVT.DATA[3] >> 24;

			DWORD size = (sizeof(XHCI_SLOT_CONTEXT) << controller->CSZ) * 32;
			XHCI_SLOT_CONTEXT *dev = 0;
			QWORD pageCount = 1;
			AllocatePhysicalMemory((QWORD *) &dev, PAGE4_4K, &pageCount);
			dev = (XHCI_SLOT_CONTEXT *) (((QWORD) dev) | SYSTEM_LINEAR);
			memset(dev, 0, size);
			controller->DVC[slot] = physical_mapping((QWORD) dev);

			// Send SET_ADDRESS command
			// Send Address Device command
			XHCI_TRB_ADDRESS_DEVICE trb01;
			memset(&trb01, 0, sizeof(XHCI_TRB_ADDRESS_DEVICE));
			trb01.ICP = physical_mapping((QWORD) icctx) >> 4;
			trb01.TYPE = TRB_ADDRESS_DEVICE;
			trb01.SID = slot;
			cc = XHCICommand(controller, &trb01.TRB);
			if (cc != 1)
			{
				OUTPUTTEXT("ADDRESS DEVICE FAILED ");
				PRINTRAX(cc, 2);
				LINEFEED();
				// Disable slot
				XHCI_TRB_DISABLE_SLOT trb02;
				memset(&trb02, 0, sizeof(XHCI_TRB_DISABLE_SLOT));
				trb02.TYPE = TRB_DISABLE_SLOT;
				trb02.SID = slot;
				cc = XHCICommand(controller, &trb02.TRB);
				if (cc != 1)
				{
					OUTPUTTEXT("DISABLE SLOT FAILED ");
					PRINTRAX(cc, 2);
					LINEFEED();
					goto FAILED;
				}
				controller->DVC[slot] = 0;
				FreePhysicalMemory(physical_mapping((QWORD) dev), PAGE4_4K, 1);
				goto FAILED;
			}
			pipe->SID = slot;
			FreePhysicalMemory(physical_mapping((QWORD) dev), PAGE4_4K, 1);
		}
		else
		{
			XHCI_PIPE *defpipe = (XHCI_PIPE *) common->PIPE;
			pipe->SID = defpipe->SID;
			// Send configure command
			XHCI_TRB_CONFIGURE_ENDPOINT trb;
			memset(&trb, 0, sizeof(XHCI_TRB_CONFIGURE_ENDPOINT));
			trb.ICP = physical_mapping((QWORD) icctx);
			trb.TYPE = TRB_CONFIGURE_ENDPOINT;
			trb.SID = pipe->SID;
			DWORD cc;
			if (cc = XHCICommand(controller, &trb.TRB) != 1)
			{
				OUTPUTTEXT("CONFIGURE ENDPOINT FAILED ");
				PRINTRAX(cc, 2);
				LINEFEED();
				goto FAILED;
			}
		}
		FreePhysicalMemory(physical_mapping((QWORD) icctx), PAGE4_4K, 1);
		return &pipe->USB;

		FAILED:;
		FreePhysicalMemory(physical_mapping((QWORD) icctx), PAGE4_4K, 1);
		FreePhysicalMemory(physical_mapping((QWORD) pipe->RING.RING), PAGE4_4K, 1);
		HeapFree(pipe);
		return 0;
	}
	BYTE eptype = epdesc->AT & USB_ENDPOINT_XFER_TYPE;
	DWORD oldmp = upipe->MPS;
	USBD2P(upipe, common, epdesc);
	XHCI_PIPE *pipe = (XHCI_PIPE *) upipe;
	XHCI_CONTROLLER *controller = (XHCI_CONTROLLER *) upipe->CTRL;
	if (eptype != USB_ENDPOINT_XFER_CONTROL || upipe->MPS == oldmp)
	{
		return upipe;
	}

	// Max Packet has changed on control endpoint - update controller
	XHCI_INPUT_CONTROL_CONTEXT *in = (XHCI_INPUT_CONTROL_CONTEXT *) XHCICreateInputContext(common, 1);
	
	in->ADD = 2;
	XHCI_ENDPOINT_CONTEXT *ep = (XHCI_ENDPOINT_CONTEXT *) (in + (2ULL << controller->CSZ));
	ep->MPSZ = pipe->USB.MPS;
	
	/********************* Necssary? *********************/
	XHCI_SLOT_CONTEXT *slot = (XHCI_SLOT_CONTEXT *) (in + (1ULL << controller->CSZ));
	DWORD port = (slot->RHPN - 1);
	DWORD portsc = controller->PR[port].PSC;
	if (!(portsc & XHCI_PORTSC_CCS))
	{
		return 0;
	}
	/********************* ********* *********************/

	XHCI_TRB_EVALUATE_CONTEXT trb;
	memset(&trb, 0, sizeof(XHCI_TRB_EVALUATE_CONTEXT));
	trb.ICP = physical_mapping((QWORD) in);
	trb.TYPE = TRB_EVALUATE_CONTEXT;
	trb.SID = pipe->SID;
	DWORD cc = XHCICommand(controller, &trb.TRB);
	if (cc != 1)
	{
		OUTPUTTEXT("CREATE PIPE:EVALUATE CONTROL ENDPOINT FAILED ");
		PRINTRAX(cc, 2);
		LINEFEED();
	}
	FreePhysicalMemory(physical_mapping((QWORD) in), PAGE4_4K, 1);
	return upipe;
}
DWORD XHCITransfer(USB_PIPE *pipe, USB_DEVICE_REQUEST *req, void *data, DWORD xferlen, DWORD wait)
{
	XHCI_PIPE *xpipe = (XHCI_PIPE *) pipe;
	XHCI_CONTROLLER *controller = (XHCI_CONTROLLER *) pipe->CTRL;
	DWORD slotid = xpipe->SID;
	XHCI_TRANSFER_RING *ring = &xpipe->RING;
	if (req)
	{
		DWORD dir = (req->T & 0x80) >> 7;
		if (req->C == USB_REQ_SET_ADDRESS)
			return 0;
		XHCI_TRB_SETUP_STAGE trb0;
		memset(&trb0, 0, sizeof(XHCI_TRB_SETUP_STAGE));
		trb0.DATA = *req;
		// memcpy(&trb0.DATA, req, sizeof(USB_DEVICE_REQUEST));
		trb0.TL = 8;
		trb0.IDT = 1;
		trb0.TYPE = TRB_SETUP_STAGE;
		trb0.TRT = req->L ? (2 | dir) : 0;
		XHCIQueueTRB(ring, &trb0.TRB);
		if (req->L)
		{
			XHCI_TRB_DATA_STAGE trb1;
			memset(&trb1, 0, sizeof(XHCI_TRB_DATA_STAGE));
			trb1.DATA = physical_mapping((QWORD) data);
			trb1.TL = req->L;
			trb1.TYPE = TRB_DATA_STAGE;
			trb1.DIR = dir;
			XHCIQueueTRB(ring, &trb1.TRB);
		}
		XHCI_TRB_STATUS_STAGE trb2;
		memset(&trb2, 0, sizeof(XHCI_TRB_STATUS_STAGE));
		trb2.TYPE = TRB_STATUS_STAGE;
		trb2.IOC = 1;
		trb2.DIR = 1 ^ dir;
		XHCIQueueTRB(ring, &trb2.TRB);
		controller->DR[slotid] = xpipe->EPID;
	}
	else
	{
		// XHCI Transfer Normal
		// while (1) __halt();
		XHCI_TRB_NORMAL trb;
		memset(&trb, 0, sizeof(XHCI_TRB_NORMAL));
		trb.DATA = physical_mapping((QWORD) data);
		trb.TL = xferlen;
		trb.IOC = 1;
		trb.TYPE = TRB_NORMAL;
		XHCIQueueTRB(ring, &trb.TRB);
		controller->DR[slotid] = xpipe->EPID;
	}
	DWORD cc = 1;
	if (!wait)
	{
		cc = XHCIWaitCompletion(controller, ring);
	}
	if (cc != 1)
	{
		return cc;
	}
	return 0;
}
void XHCICreateTransferRing(XHCI_TRANSFER_RING *tr)
{
	QWORD pageAddress = 0;
	QWORD pageCount = 1;
	AllocatePhysicalMemory(&pageAddress, PAGE4_4K, &pageCount);
	pageAddress |= SYSTEM_LINEAR;
	memset((void *) pageAddress, 0, 4096);
	tr->RING = (XHCI_TRANSFER_BLOCK *) pageAddress;
	XHCI_TRB_TRANSFER_RING *ring = (XHCI_TRB_TRANSFER_RING *) (tr->RING + XHCI_RING_ITEMS);
	ring->RING = physical_mapping((QWORD) tr);
}
