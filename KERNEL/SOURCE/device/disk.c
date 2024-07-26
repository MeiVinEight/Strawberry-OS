#include <device/disk.h>
#include <intrinsic.h>

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
	if (ret && op->CNT == origcnt) op->CNT = 0;
	return ret;
}
int DefaultDiskOperation(DISK_OPERATION *op)
{
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
DWORD DISKRW(DISK_DRIVER *drive, void *data, QWORD lba, WORD count, BYTE cmd)
{
	DISK_OPERATION dop;
	memset(&dop, 0, sizeof(DISK_OPERATION));
	dop.DRV = drive;
	dop.CMD = cmd;
	dop.CNT = count;
	dop.LBA = lba;
	dop.DAT = data;
	return ExecuteDiskOperation(&dop);
}