#include <device/ahci.h>
#include <memory/heap.h>
#include <intrinsic.h>
#include <console/console.h>
#include <system.h>
#include <timer/timer.h>
#include <memory/page.h>
#include <common/string.h>

#define AHCI_GHC_RST  (0x00000001) /* reset controller; self-clear */
#define AHCI_GHC_IE   (0x00000002) /* global IRQ enable */
#define AHCI_GHC_AE   (0x80000000) /* AHCI enabled */

#define AHCI_PORT_CMD_ST  0x00000001 // Start
#define AHCI_PORT_CMD_SUD 0x00000002 // Spin-Up Device
#define AHCI_PORT_CMD_POD 0x00000004 // Power On Device
#define AHCI_PORT_CMD_CLO 0x00000008 // Command List Override
#define AHCI_PORT_CMD_FRE 0x00000010 // FIS Receive Enable
#define AHCI_PORT_CMD_FR  0x00004000 // FIS Receive Running
#define AHCI_PORT_CMD_CR  0x00008000 // Command List Running

#define AHCI_PORT_IPM_ACTIVE  1
#define AHCI_PORT_DET_PRESENT 3

#define AHCI_PORT_DEV_BUSY 0x80
#define AHCI_PORT_DEV_DRQ  0x08

#define AHCI_PORT_IST_TFES 0x40000000 // Task File Error Status

#define	AHCI_PORT_SIG_ATA    0x00000101  // SATA drive
#define	AHCI_PORT_SIG_ATAPI  0xEB140101  // SATAPI drive
#define	AHCI_PORT_SIG_SEMB   0xC33C0101  // Enclosure management bridge
#define	AHCI_PORT_SIG_PM     0x96690101  // Port multiplier

#define ATA_CMD_READ_DMA_EX                 0x25
#define ATA_CMD_WRITE_DMA_EXT               0x35
#define ATA_CMD_IDENTIFY_DEVICE             0xEC

void ReverseWord(void *p, QWORD cnt)
{
	BYTE(*buffer)[2] = (BYTE(*)[2]) p;
	for (DWORD i = 0; i < cnt; i++)
	{
		BYTE c = buffer[i][0];
		buffer[i][0] = buffer[i][1];
		buffer[i][1] = c;
	}
}
void AHCICommandRST(AHCI_HBA_PORT *port)
{
	// Clear ST
	port->CMD &= ~AHCI_PORT_CMD_ST;
	// Clear FRE
	port->CMD &= ~AHCI_PORT_CMD_FRE;
	// Wait FR and CR clear
	while (port->CMD & (AHCI_PORT_CMD_FR | AHCI_PORT_CMD_CR)) __halt();
}
DWORD AHCICommandEN(AHCI_HBA_PORT *port)
{
	AHCICommandRST(port);

	DWORD cmd = port->CMD;
	cmd |= AHCI_PORT_CMD_FRE;
	cmd |= AHCI_PORT_CMD_SUD;
	cmd |= AHCI_PORT_CMD_ST;
	port->CMD = cmd;
	// Wait for LINK UP
	QWORD timeout = TimestampCPU() + 10; // AHCI_LINK_TIMEOUT
	while (1)
	{
		if ((port->SAS & 7) == 3) return 0;
		if (TimestampCPU() > timeout) break;
	}
	OUTPUTTEXT("AHCI PORT LINK DOWN\n");
	return -1;
}
DWORD AHCICommandWR(DWORD cmd)
{
	switch (cmd)
	{
		case ATA_CMD_WRITE_DMA_EXT: return 1;
	}
	return 0;
}
DWORD OperationATA(AHCI_HBA_PORT *port, QWORD lba, WORD count, DWORD cmd, void *buf)
{
	if (!count) return 0;
	if (count > 128) count = 128;
	if (cmd == ATA_CMD_IDENTIFY_DEVICE) count = 1;

	port->IST = -1;
	int slot = 0; // SearchCMD

	AHCI_COMMAND_HEAD *cmdh = (AHCI_COMMAND_HEAD *) (port->CLB | SYSTEM_LINEAR);
	cmdh += slot;
	cmdh->CFL = 5; // sizeof(FIS_REG_H2D) / sizeof(DWORD)
	cmdh->WRT = AHCICommandWR(cmd);
	cmdh->DTL = ((count - 1) >> 4) + 1; // UPPER BOUND (count / 16)

	AHCI_COMMAND_TABLE *tbl = (AHCI_COMMAND_TABLE *) ((cmdh->TBL) | SYSTEM_LINEAR);
	memset(tbl, 0, sizeof(AHCI_COMMAND_TABLE) + (cmdh->DTL * sizeof(AHCI_PRDT_ENTRY)));

	QWORD bufAddr = physical_mapping((QWORD) buf);
	WORD count1 = count;
	DWORD i = 0;
	while (count1)
	{
		DWORD cnt = (count1 < 16) ? count1 : 16;
		tbl->PRD[i].DBA = bufAddr;
		tbl->PRD[i].DBC = (cnt << 9) - 1;
		tbl->PRD[i].IOC = 0; // Wrong ?
		count1 -= cnt;
		bufAddr += ((QWORD) cnt) << 9;
		i++;
	}

	AHCI_FIS_H2D *fis = &tbl->FIS;
	memset(fis, 0, sizeof(AHCI_FIS_H2D));

	fis->TYP = 0x27; // H2D
	fis->CCC = 1; // Command
	fis->CMD = cmd;
	fis->FTL = 1;

	fis->BA0 = (lba >> 0x00) & 0xFF;
	fis->BA1 = (lba >> 0x08) & 0xFF;
	fis->BA2 = (lba >> 0x10) & 0xFF;
	fis->DVC = 1 << 6; // LBA MODE
	fis->BA3 = (lba >> 0x18) & 0xFF;
	fis->BA4 = (lba >> 0x20) & 0xFF;
	fis->BA5 = (lba >> 0x28) & 0xFF;

	fis->CNT = count;

	port->SAE = port->SAE;
	// The below loop waits until the prot is no longer busy before issuing a new command
	QWORD timeout = TimestampCPU() + 10000; // Waiting 10s
	while ((port->TFD & (AHCI_PORT_DEV_DRQ | AHCI_PORT_DEV_BUSY)))
	{
		if (TimestampCPU() > timeout)
		{
			OUTPUTTEXT("AHCI WAIT DEVICE BSY TIMEOUT\n");
			return 1 << 16; // Timeout
		}
	}
	port->CIS = 1 << slot;

	// Wait for completion
	timeout = TimestampCPU() + 10000; // Waiting 10s
	while ((port->CIS & (1 << slot)) && !(port->IST & AHCI_PORT_IST_TFES));
	if (port->IST & AHCI_PORT_IST_TFES)
	{
		return 2 << 16;
	}

	return count;
}
DWORD OperationAHCI(DISK_OPERATION *op)
{
	AHCI_PORT *port = (AHCI_PORT *) op->DRV;
	DWORD cc = DISK_RET_SUCCESS;
	switch (op->CMD)
	{
		case CMD_IDENTIFY:
		{
			op->CNT = 1;
			DISK_IDENTIFY *idfy = (DISK_IDENTIFY *) op->DAT;
			memset(idfy, 0, sizeof(DISK_IDENTIFY));
			memcpy(idfy->SER, port->SER, sizeof(port->SER));
			memcpy(idfy->MOD, port->MOD, sizeof(port->MOD));
			break;
		}
		case CMD_READ:
		case CMD_WRITE:
		{
			if (op->CNT > (4096 / port->DRV.BS)) op->CNT = (4096 / port->DRV.BS);
			// QWORD pagAddr = 0;
			// QWORD pagCont = 1;
			// AllocatePhysicalMemory(&pagAddr, 0, &pagCont);
			// BYTE *buf = (BYTE *) (pagAddr | SYSTEM_LINEAR);
			// memset(buf, 0, 4096);
			// for (WORD i = 0; i < op->CNT;)
			{
				// WORD remaining = op->CNT - i;
				// if (remaining > (4096 / port->DRV.BS)) remaining = (4096 / port->DRV.BS);
				// BYTE *opbuf = ((BYTE *) op->DAT) + (i * port->DRV.BS);
				DWORD cmd = 0;

				// DISK_CMD cmd to ATA CMD
				switch (op->CMD)
				{
					case CMD_READ:
					{
						cmd = ATA_CMD_READ_DMA_EX;
						break;
					}
					case CMD_WRITE:
					{
						cmd = ATA_CMD_WRITE_DMA_EXT;
						break;
					}
				}
				DWORD isWrite = AHCICommandWR(cmd);
				// if (isWrite) memcpy(buf, opbuf, remaining * port->DRV.BS);
				DWORD blocks = OperationATA(port->PRT, op->LBA, op->CNT, cmd, op->DAT);
				if (blocks >> 16)
				{
					op->CNT = 0;
					cc = DISK_RET_EBADTRACK;
					goto AHCI_OVER;
				}
				op->CNT = blocks;
				// if (!isWrite) memcpy(opbuf, buf, blocks * port->DRV.BS);
				// i += blocks;
			}
			AHCI_OVER:;
			// FreePhysicalMemory(pagAddr, PAGE4_4K, 1);
			break;
		}
		default: cc = DefaultDiskOperation(op);
	}
	return cc;
}
void ConfigureAHCI(PCI_DEVICE *dvc)
{
	OutputPCIDevice(dvc->CMD);

	QWORD iobase = PCIEnableMMIO(dvc->CMD, PCI_BASE_ADDRESS_5);
	PCIEnableBusMaster(dvc->CMD);

	AHCI_CONTROLLER *ctrl = (AHCI_CONTROLLER *) HeapAlloc(HEAPK, sizeof(AHCI_CONTROLLER));
	memset(ctrl, 0, sizeof(AHCI_CONTROLLER));

	ctrl->HBA = (AHCI_HBA_MEMORY *) iobase;

	ctrl->HBA->GHC |= AHCI_GHC_AE;
	DWORD pi = ctrl->HBA->PTI;
	DWORD pn = 0;
	while (pi)
	{
		if (pi & 1)
		{
			AHCI_HBA_PORT *port = &(ctrl->HBA->PRT[pn]);
			DWORD sas = port->SAS;
			BYTE ipm = (sas >> 8) & 0xF;
			BYTE det = (sas >> 0) & 0xF;
			if (det != AHCI_PORT_DET_PRESENT || ipm != AHCI_PORT_IPM_ACTIVE) goto AHCI_NEXT_PORT;

			if (port->SIG == AHCI_PORT_SIG_ATA)
			{
				// Maybe not need to realloc
				if (AHCICommandEN(port))
				{
					OUTPUTTEXT("CANNOT LINK UP PORT ");
					PRINTRAX(port - ctrl->HBA->PRT, 2);
					LINEFEED();
					AHCICommandRST(port);
					goto AHCI_NEXT_PORT;
				}
				BYTE *buf = 0;
				QWORD pagCont = 1;
				AllocatePhysicalMemory((QWORD *) &buf, PAGE4_4K, &pagCont);
				buf = (BYTE *) (((QWORD) buf) | SYSTEM_LINEAR);
				memset(buf, 0, 4096);
				DWORD cc = OperationATA(port, 0, 1, ATA_CMD_IDENTIFY_DEVICE, buf);
				if (cc >> 16)
				{
					OUTPUTTEXT("CANNOT READ AHCI DISK ");
					PRINTRAX(cc >> 16, 4);
					LINEFEED();
					while (1) __halt();
				}

				AHCI_PORT *ap = HeapAlloc(HEAPK, sizeof(AHCI_PORT));
				memset(ap, 0, sizeof(AHCI_PORT));
				ap->DRV.DVR.TYPE = DTYPE_AHCI_SATA;
				ap->DRV.OP = OperationAHCI;
				ap->DRV.SCT = *((QWORD *) (buf + 200));
				ap->DRV.ID = port - ctrl->HBA->PRT;
				ap->DRV.BS = 0x200; // 512
				ap->CTRL = ctrl;
				ap->PRT = port;
				ap->PNR = ap->DRV.ID;

				/*
				PRINTRAX(port - ctrl->HBA->PRT, 2);
				OUTCHAR(':');
				OUTCHAR('[');
				*/

				BYTE *identify = (BYTE *) (buf + 0x14);
				identify[0x14] = 0;
				ReverseWord(identify, 0x0A);
				identify = LeadingWhitespace(identify, identify + 0x14);
				memcpy(ap->SER, identify, strlen(identify));

				/*
				OUTPUTTEXT(ap->SER);
				OUTCHAR(']');
				OUTCHAR('[');
				*/

				identify = (BYTE *) (buf + 0x36);
				identify[0x28] = 0;
				ReverseWord(identify, 0x14);
				identify = LeadingWhitespace(identify, identify + 0x28);
				memcpy(ap->MOD, identify, strlen(identify));

				LinkupDisk((DISK_DRIVER *) ap);

				/*
				OUTPUTTEXT(ap->MOD);
				OUTCHAR(']');
				PRINTRAX(ap->DRV.SCT, 16);
				LINEFEED();
				*/
				// Try to read sector
				/*
				cc = OperationATA(port, 1, 1, ATA_CMD_READ_DMA_EX, buf);
				if (cc >> 16)
				{
					OUTPUTTEXT("CANNOT READ AHCI DISK ");
					PRINTRAX(cc >> 16, 4);
					LINEFEED();
					while (1) __halt();
				}
				((QWORD *) buf)[1] = '\n';
				PRINTRAX(*((QWORD *) buf), 16);
				LINEFEED();

				memset(buf, 0, 4096);
				DISK_OPERATION dop;
				memset(&dop, 0, sizeof(DISK_OPERATION));
				dop.DRV = (DISK_DRIVER *) ap;
				dop.CMD = CMD_READ;
				dop.CNT = 8;
				dop.DAT = buf;
				dop.LBA = 0;
				ExecuteDiskOperation(&dop);
				PRINTRAX(*((QWORD *) (buf + 512)), 16);
				LINEFEED();
				*/

				FreePhysicalMemory(physical_mapping((QWORD) buf), PAGE4_4K, 1);
			}
		}
		AHCI_NEXT_PORT:;
		pi >>= 1;
		pn++;
	}
}