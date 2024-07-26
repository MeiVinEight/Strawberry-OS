#include <device/scsi.h>
#include <intrinsic.h>
#include <console/console.h>
#include <timer/timer.h>
#include <memory/page.h>
#include <system.h>

int SCSIWaitReady(DISK_OPERATION *op)
{
	int tries = 3;
	int inProgress = 0;
	QWORD end = TimestampCPU() + 5000; // +5s
	while (1)
	{
		if (TimestampCPU() >= end)
		{
			OUTPUTTEXT("TEST UNIT READY FAILED\n");
			return -1;
		}
		
		SCSI_CMD_TEST_UNIT_READY cmd0;
		memset(&cmd0, 0, sizeof(SCSI_CMD_TEST_UNIT_READY));
		cmd0.CODE = CDB_CMD_TEST_UNIT_READY;
		op->CMD = CMD_SCSI;
		op->CNT = 0;
		op->DAT = 0;
		op->CDB = &cmd0;
		op->BSZ = 0;
		debug = 1;
		if (!ExecuteDiskOperation(op)) break;

		SCSI_DAT_REQUEST_SENSE sense;
		memset(&sense, 0, sizeof(SCSI_DAT_REQUEST_SENSE));
		SCSI_CMD_REQUEST_SENSE cmd1;
		memset(&cmd1, 0, sizeof(SCSI_CMD_REQUEST_SENSE));
		cmd1.CODE = CDB_CMD_REQUEST_SENSE;
		cmd1.AL = sizeof(SCSI_DAT_REQUEST_SENSE);
		op->CMD = CMD_SCSI;
		op->CNT = 1;
		op->DAT = &sense;
		op->CDB = &cmd1;
		op->BSZ = sizeof(SCSI_DAT_REQUEST_SENSE);
		if (ExecuteDiskOperation(op))
		{
			OUTPUTTEXT("SENSE FAILED\n");
			continue;
		}

		// Sense success
		if (sense.ASC == SCSI_ASC_MEDIUM_NOT_PRESENT)
		{
			tries--;
			OUTPUTTEXT("DEVICE REPORT MEDIUM NOT PRESENT ");
			PRINTRAX(tries, 1);
			OUTPUTTEXT(" TRIES LEFT\n");
			if (!tries) return -1;
		}

		if (sense.ASC == SCSI_ASC_LOGICAL_UNIT_NOT_READY && sense.ASQ == SCSI_ASQ_IN_PROGRESS && !inProgress)
		{
			OUTPUTTEXT("WAITING FOR DEVICE TO DETECT MEDIUM 30S\n");
			end += 30;
			inProgress = 1;
		}
	}
	return 0;
}
int SCSISetup(DISK_DRIVER *drive)
{
	DISK_OPERATION dop;
	memset(&dop, 0, sizeof(DISK_OPERATION));
	dop.DRV = drive;
	SCSI_DAT_INQUIRY data;
	SCSI_CMD_INQUIRY cmd;
	memset(&cmd, 0, sizeof(SCSI_CMD_INQUIRY));
	memset(&data, 0, sizeof(SCSI_DAT_INQUIRY));
	cmd.CODE = CDB_CMD_INQUIRY;
	cmd.AL = __reverse16(sizeof(SCSI_DAT_INQUIRY));
	dop.CMD = CMD_SCSI;
	dop.CNT = 1;
	dop.DAT = &data;
	dop.CDB = &cmd;
	dop.BSZ = sizeof(SCSI_DAT_INQUIRY);
	int cc = ExecuteDiskOperation(&dop);
	if (cc)
	{
		OUTPUTTEXT("CANNOT INQUERY SCSI\n");
		return cc;
	}
	/*
	char buf[17];
	memcpy(buf, data.VEN, sizeof(data.VEN));
	buf[sizeof(data.VEN)] = 0;
	OUTPUTTEXT("VENDER  :");
	OUTPUTTEXT(buf);
	LINEFEED();
	memcpy(buf, data.PROD, sizeof(data.PROD));
	buf[sizeof(data.PROD)] = 0;
	OUTPUTTEXT("PRODUCT :");
	OUTPUTTEXT(buf);
	LINEFEED();
	memcpy(buf, data.REV, sizeof(data.REV));
	buf[sizeof(data.REV)] = 0;
	OUTPUTTEXT("REVISION:");
	OUTPUTTEXT(buf);
	LINEFEED();
	memcpy(buf, data.SRA, sizeof(data.SRA));
	buf[sizeof(data.SRA)] = 0;
	OUTPUTTEXT("SERIAL  :");
	OUTPUTTEXT(buf);
	LINEFEED();
	*/

	BYTE pdt = data.PDT;
	BYTE removable = data.RMV;
	drive->RMV = removable;

	if (pdt != SCSI_TYPE_DISK) return -1;

	if (SCSIWaitReady(&dop))
	{
		OUTPUTTEXT("SCSI NOT READY\n");
		return -1;
	}

	SCSI_CMD_READ_CAPACITY cmd1;
	SCSI_DAT_READ_CAPACITY capa;
	memset(&cmd1, 0, sizeof(SCSI_CMD_READ_CAPACITY));
	memset(&capa, 0, sizeof(SCSI_DAT_READ_CAPACITY));
	cmd1.CODE = CDB_CMD_READ_CAPACITY;
	dop.CMD = CMD_SCSI;
	dop.CNT = 1;
	dop.DAT = &capa;
	dop.CDB = &cmd1;
	dop.BSZ = sizeof(SCSI_DAT_READ_CAPACITY);
	cc = ExecuteDiskOperation(&dop);
	if (cc)
	{
		OUTPUTTEXT("CANNOT READ CAPACITY ");
		PRINTRAX(cc, 2);
		LINEFEED();
		return -1;
	}

	drive->BS = __reverse32(capa.BSZ);
	if (drive->BS != 512)
	{
		OUTPUTTEXT("NOT A NORMAL DISK:SECTOR SIZE = ");
		OUTPUTWORD(capa.BSZ);
		LINEFEED();
		return -1;
	}
	drive->SCT = __reverse32(capa.LBA) + 1;

	/*
	OUTPUTTEXT("DISK CAPACITY ");
	PRINTRAX(drive->SCT << 9, 16);
	LINEFEED();
	*/
	
	// Try to read sector
	/*
	BYTE *sector = 0;
	QWORD pageCount = 1;
	AllocatePhysicalMemory((QWORD *) &sector, PAGE4_4K, &pageCount);
	sector = (BYTE *) (((QWORD) sector) | SYSTEM_LINEAR);
	memset(sector, 0, 4096);
	cc = DISKRW(drive, sector, 2048, 1, CMD_READ);
	if (cc)
	{
		OUTPUTTEXT("CANNOT READ SECTOR ");
		PRINTRAX(cc, 2);
		LINEFEED();
		return -1;
	}
	OUTPUTTEXT(sector + 3);
	OUTCHAR(' ');
	cc = DISKRW(drive, sector, 15728631, 1, CMD_READ);
	PRINTRAX(*((QWORD *) sector), 16);
	LINEFEED();
	*((QWORD *) sector) = 0xFEEB;
	cc = DISKRW(drive, sector, 15728631, 1, CMD_WRITE);
	FreePhysicalMemory((QWORD) sector, PAGE4_4K, 1);
	if (cc)
	{
		OUTPUTTEXT("CANNOT WRITE SECTOR ");
		PRINTRAX(cc, 2);
		LINEFEED();
		return -1;
	}
	while (1) __halt();
	*/


	return 0;
}
int SCSISetupCommand(DISK_OPERATION *op, void *cdbcmd, int maxcdb)
{
	switch (op->CMD)
	{
		case CMD_READ:
		{
			SCSI_CMD_READ_10 *read = cdbcmd;
			memset(read, 0, sizeof(SCSI_CMD_READ_10));
			read->CODE = CDB_CMD_READ_10;
			read->LBA = __reverse32(op->LBA);
			read->TL = __reverse16(op->CNT);
			return op->DRV->BS;
		}
		case CMD_WRITE:
		{
			SCSI_CMD_WRITE_10 *cmd = cdbcmd;
			memset(cmd, 0, sizeof(SCSI_CMD_WRITE_10));
			cmd->CODE = CDB_CMD_WRITE_10;
			cmd->LBA = __reverse32(op->LBA);
			cmd->TL = __reverse16(op->CNT);
			return op->DRV->BS;
		}
		case CMD_SCSI:
		{
			memcpy(cdbcmd, op->CDB, maxcdb);
			return op->BSZ;
		}
		default:
		{
			OUTPUTTEXT("SETUP COMMAND ");
			PRINTRAX(op->CMD, 2);
			LINEFEED();
			return -1;
		}
	}
}