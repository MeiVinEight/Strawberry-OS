#include <device/usb/msc.h>
#include <console/console.h>
#include <memory/heap.h>
#include <intrinsic.h>
#include <device/scsi.h>
#include <memory/page.h>
#include <system.h>

int MSCSendCBW(USB_DISK_DRIVER *udrive, int dir, void *buf, DWORD bytes)
{
	USB_PIPE *pipe = (dir == USB_DIR_OUT) ? udrive->BOP : udrive->BIP;
	return pipe->CTRL->XFER(pipe, 0, buf, bytes, 0);
}
DWORD OperationMSC(DISK_OPERATION *op)
{
	USB_DISK_DRIVER *udrive = (USB_DISK_DRIVER *) op->DRV;
	if (op->CMD == CMD_IDENTIFY)
	{
		// IDENTIFY
		op->CNT = 1;
		memset(op->DAT, 0, sizeof(DISK_IDENTIFY));
		DISK_IDENTIFY *identify = (DISK_IDENTIFY *) op->DAT;
		memcpy(identify->MOD, udrive->DVR.MOD, sizeof(udrive->DVR.MOD));
		return DISK_RET_SUCCESS;
	}

	/*
	OUTPUTTEXT("USB DISK R/W LBA=");
	PRINTRAX(op->LBA, 8);
	OUTPUTTEXT(" CNT=");
	PRINTRAX(op->CNT, 4);
	OUTPUTTEXT(" DAT=");
	PRINTRAX((QWORD) op->DAT, 16);
	LINEFEED();
	*/

	int blocksize = -1;
	if (op->CMD == CMD_SCSI)
	{
		blocksize = op->BSZ;
	}
	else if (op->CMD == CMD_READ || op->CMD == CMD_WRITE)
	{
		blocksize = op->DRV->BS;
	}
	if (blocksize && (op->CNT > (4096 / blocksize))) op->CNT = (4096 / blocksize);
	BYTE dir = (op->CMD == CMD_READ || (op->CMD == CMD_SCSI && op->BSZ)) ? USB_DIR_IN : USB_DIR_OUT;

	// Setup command block wrapper
	USB_COMMAND_BLOCK_WRAPPER cbw;
	memset(&cbw, 0, sizeof(USB_COMMAND_BLOCK_WRAPPER));
	SCSISetupCommand(op, cbw.CMD, USB_CDB_SIZE);
	if (blocksize < 0)
	{
		OUTPUTTEXT("CANNOT SETUP CBW\n");
		return DefaultDiskOperation(op);
	}

	DWORD bytes = blocksize * op->CNT;

	/*
	OUTPUTTEXT("BYTE ");
	PRINTRAX(bytes, 8);
	LINEFEED();
	*/

	cbw.SIG = CBW_SIGNATURE;
	cbw.TAG = 999;
	cbw.DTL = bytes;
	cbw.FLG = dir;
	cbw.LUN = udrive->LUN;
	cbw.CBL = USB_CDB_SIZE;

	// Transfer cbw to device
	int cc = MSCSendCBW(udrive, USB_DIR_OUT, &cbw, 31);
	if (cc)
	{
		OUTPUTTEXT("CANNOT TRANSFER CBW\n");
		goto MSC_FAILED;
	}

	// Transfer data to/from device
	if (bytes)
	{
		cc = MSCSendCBW(udrive, dir, op->DAT, bytes);
		if (cc)
		{
			OUTPUTTEXT("CANNOT TRANSFER DATA\n");
			goto MSC_FAILED;
		}
	}

	// Transfer CSW info
	USB_COMMAND_STATUS_WRAPPER csw;
	memset(&csw, 0, sizeof(USB_COMMAND_STATUS_WRAPPER));
	cc = MSCSendCBW(udrive, USB_DIR_IN, &csw, sizeof(USB_COMMAND_STATUS_WRAPPER));
	if (cc)
	{
		OUTPUTTEXT("CANNOT TRANSFER CSW\n");
		goto MSC_FAILED;
	}

	if (!csw.STS) return DISK_RET_SUCCESS;
	if (csw.STS == 2) goto MSC_FAILED;
	if (blocksize) op->CNT -= csw.RSD / blocksize;

	MSC_FAILED:;
	OUTPUTTEXT("USB TRANSMISSION FAILED\n");
	return DISK_RET_EBADTRACK;
}
int SetupMSCLUN(USB_COMMON *usbdev, USB_PIPE *ipipe, USB_PIPE *opipe, int lun)
{
	USB_DISK_DRIVER *drive = HeapAlloc(HEAPK, sizeof(USB_DISK_DRIVER));
	memset(drive, 0, sizeof(USB_DISK_DRIVER));
	usbdev->DRV = (DEVICE_DRIVER *) drive;

	if (ipipe->CTRL->TYPE == USB_TYPE_XHCI)
	{
		drive->DVR.DRV.DVR.TYPE = DTYPE_USB_MSC_32;
	}
	else
	{
		drive->DVR.DRV.DVR.TYPE = DTYPE_USB_MSC;
	}
	drive->DVR.DRV.OP = OperationMSC;
	drive->BIP = ipipe;
	drive->BOP = opipe;
	drive->LUN = lun;

	if (SCSISetup(&(drive->DVR)))
	{
		OUTPUTTEXT("CANNOT CONFIGURE USB MSC DEVICE\n");
		return -1;
	}
	LinkupDisk((DISK_DRIVER *) drive);

	/*
	BYTE *sector = 0;
	QWORD pageCount = 1;
	AllocatePhysicalMemory((QWORD *) &sector, PAGE4_4K, &pageCount);
	sector = (BYTE *) (((QWORD) sector) | SYSTEM_LINEAR);
	memset(sector, 0, 4096);
	int cc = DISKRW((DISK_DRIVER *) drive, sector, 1, 1, CMD_READ);
	if (cc)
	{
		OUTPUTTEXT("CANNOT READ SECTOR ");
		PRINTRAX(cc, 2);
		LINEFEED();
		return -1;
	}
	PRINTRAX(*((QWORD *) sector), 16);
	// OUTPUTTEXT(sector);
	OUTCHAR('\n');
	FreePhysicalMemory(physical_mapping((QWORD) sector), PAGE4_4K, 1);
	*/

	/*
	QWORD pagAddr = 0;
	QWORD pagCont = 1;
	AllocatePhysicalMemory(&pagAddr, PAGE4_4K, &pagCont);
	BYTE *pageBuffer = (BYTE *) (pagAddr | SYSTEM_LINEAR);
	memset(pageBuffer, 0, 4096);

	DISK_OPERATION dop;
	memset(&dop, 0, sizeof(DISK_OPERATION));
	dop.DRV = (DISK_DRIVER *) drive;
	dop.CMD = CMD_READ;
	dop.LBA = 0;
	dop.CNT = 8;
	dop.DAT = pageBuffer;
	ExecuteDiskOperation(&dop);

	PRINTRAX(*((QWORD *) (pageBuffer + 512)), 16);
	LINEFEED();
	FreePhysicalMemory(pagAddr, PAGE4_4K, 1);
	*/

	return 0;
}
int ConfigureMSC(USB_COMMON *common, USB_INTERFACE *iface)
{
	// Find bulk in and bulk out endpoints
	USB_PIPE *ipipe = 0;
	USB_PIPE *opipe = 0;

	USB_ENDPOINT *idesc = USBSearchEndpoint(common, USB_ENDPOINT_XFER_BULK, USB_DIR_IN);
	USB_ENDPOINT *odesc = USBSearchEndpoint(common, USB_ENDPOINT_XFER_BULK, USB_DIR_OUT);
	if (!idesc || !odesc) goto FAIL_MSC;

	ipipe = common->CTRL->CPIP(common, 0, idesc);
	opipe = common->CTRL->CPIP(common, 0, odesc);
	if (!ipipe || !opipe) goto FAIL_MSC;

	USB_DEVICE_REQUEST req;
	req.T = USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE;
	req.C = 0xFE; // ?
	req.V = 0;
	req.I = 0;
	req.L = 1;
	BYTE maxlun = 0;
	int cc = common->CTRL->XFER(common->PIPE, &req, &maxlun, 0, 0);
	if (cc) maxlun = 0;

	int pipesused = 0;
	for (int lun = 0; lun < maxlun + 1; lun++)
	{
		if (!SetupMSCLUN(common, ipipe, opipe, lun)) pipesused = 1;
	}

	if (!pipesused) goto FAIL_MSC;

	return 0;

	FAIL_MSC:;
	OUTPUTTEXT("CANNOT CONFIGURE USB MSC DEVICE\n");
	common->CTRL->CPIP(common, ipipe, 0);
	common->CTRL->CPIP(common, opipe, 0);
	return -1;
}