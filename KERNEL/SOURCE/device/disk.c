#include <device/disk.h>
#include <intrinsic.h>
#include <console/console.h>
#include <memory/page.h>
#include <system.h>
#include <memory/heap.h>

CODEDECL DISK_DRIVER *DISKDVC = 0;

DWORD ExecuteDiskOperation(DISK_OPERATION *op)
{
	int ret = 0;
	int origcnt = op->CNT;
	if (origcnt * (op->DRV->BS) > 65536)
	{
		op->CNT = 0;
		return DISK_RET_EBOUNDARY;
	}

	ret = op->DRV->OP(op);
	// if (ret && op->CNT == origcnt) op->CNT = 0;
	return ret;
}
int DefaultDiskOperation(DISK_OPERATION *op)
{
	op->CNT = 0;
	switch (op->CMD) {
		case CMD_FORMAT:
		case CMD_RESET:
		case CMD_ISREADY:
		case CMD_VERIFY:
		case CMD_SEEK:
			// Return success if the driver doesn't implement these commands
			return DISK_RET_SUCCESS;
		default:
			return DISK_RET_EPARAM;
	}
}
DWORD OperationWR(DWORD cmd)
{
	if (cmd == CMD_WRITE) return 1;
	return 0;
}
DWORD DISKRW(DISK_DRIVER *drive, void *data, QWORD lba, WORD count, BYTE cmd)
{
	QWORD blocksize = 0x200;
	if (cmd == CMD_IDENTIFY)
	{
		blocksize = sizeof(DISK_IDENTIFY);
		count = 1;
	}

	DWORD isWrite = OperationWR(cmd);
	BYTE *buf = (BYTE *) data;
	DWORD cc = DISK_RET_SUCCESS;
	QWORD pagAddr = 0;
	QWORD pagCont = 1;
	AllocatePhysicalMemory(&pagAddr, PAGE4_4K, &pagCont);
	BYTE *pageBuf = (BYTE *) (pagAddr | SYSTEM_LINEAR);
	for (WORD i = 0; i < count;)
	{
		QWORD currentCNT = count - i;
		if (currentCNT > 8) currentCNT = 8;
		QWORD currentLBA = lba + i;
		BYTE *currentBuf = buf + (i * blocksize);

		DISK_OPERATION dop;
		memset(&dop, 0, sizeof(DISK_OPERATION));
		dop.DRV = drive;
		dop.CMD = cmd;
		dop.CNT = currentCNT;
		dop.LBA = currentLBA;
		dop.DAT = pageBuf;
		if (isWrite) memcpy(pageBuf, currentBuf, currentCNT * blocksize);
		cc = ExecuteDiskOperation(&dop);
		if (cc != DISK_RET_SUCCESS) break;
		if (!isWrite) memcpy(currentBuf, pageBuf, (QWORD) dop.CNT * blocksize);
		i += dop.CNT;
	}
	FreePhysicalMemory(pagAddr, PAGE4_4K, pagCont);
	return cc;
}
void LinkupDisk(DISK_DRIVER *disk)
{
	disk->NXT = DISKDVC;
	disk->PRV = 0;
	if (DISKDVC)
	{
		DISKDVC->PRV = disk;
	}
	DISKDVC = disk;
}
DWORD LoadingBOOT(DISK_DRIVER *disk)
{
	
	DISK_IDENTIFY identify;
	DISKRW(disk, &identify, 0, 1, CMD_IDENTIFY);
	OUTPUTTEXT(identify.MOD);
	LINEFEED();
	

	return 0;
}
void ConfigureDSK()
{
	DISK_DRIVER *disk = DISKDVC;
	while (disk)
	{
		if (LoadingBOOT(disk)) return;
		disk = disk->NXT;
	}
}