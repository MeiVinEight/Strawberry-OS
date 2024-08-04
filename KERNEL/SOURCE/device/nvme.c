#include <device/nvme.h>
#include <device/pci.h>
#include <console/console.h>
#include <memory/heap.h>
#include <intrinsic.h>
#include <timer/timer.h>
#include <memory/page.h>
#include <system.h>
#include <declspec.h>
#include <common/string.h>
#include <device/disk.h>

#define NVME_CSTS_FATAL   (1U <<  1)
#define NVME_CSTS_RDY     (1U <<  0)

#define NVME_SQE_OPC_ADMIN_CREATE_IO_SQ 1U
#define NVME_SQE_OPC_ADMIN_CREATE_IO_CQ 5U
#define NVME_SQE_OPC_ADMIN_IDENTIFY     6U

#define NVME_SQE_OPC_IO_WRITE 1U
#define NVME_SQE_OPC_IO_READ  2U

#define NVME_ADMIN_IDENTIFY_CNS_ID_NS       0x00U
#define NVME_ADMIN_IDENTIFY_CNS_ID_CTRL     0x01U
#define NVME_ADMIN_IDENTIFY_CNS_ACT_NSL     0x02U

CODEDECL void *NVME_DMA_MEMORY = 0;

void NVMEConfigureQ(NVME_CONTROLLER *ctrl, NVME_QUEUE_COMMON *q, DWORD idx, DWORD len)
{
	memset(q, 0, sizeof(NVME_QUEUE_COMMON));
	q->DBL = (DWORD *) (((BYTE *) ctrl->CAP) + 0x1000 + idx * ctrl->DST);
	q->MSK = len - 1;
}
int NVMEConfigureCQ(NVME_CONTROLLER *ctrl, NVME_COMPLETION_QUEUE *cq, DWORD idx, DWORD len)
{
	NVMEConfigureQ(ctrl, &cq->COM, idx, len);
	cq->CQE = 0;
	QWORD phyAddr = 0;
	QWORD pagCont = 1;
	AllocatePhysicalMemory(&phyAddr, PAGE4_4K, &pagCont);
	cq->CQE = (NVME_COMPLETION_QUEUE_ENTRY *) (phyAddr | SYSTEM_LINEAR);
	memset(cq->CQE, 0, 4096);
	cq->COM.HAD = 0;
	cq->COM.TAL = 0;
	cq->COM.PHA = 1;
	return 0;
}
int NVMEConfigureSQ(NVME_CONTROLLER *ctrl, NVME_SUBMISSION_QUEUE *sq, DWORD idx, DWORD len)
{
	NVMEConfigureQ(ctrl, &sq->COM, idx, len);
	sq->SQE = 0;
	QWORD phyAddr = 0;
	QWORD pagCont = 1;
	AllocatePhysicalMemory(&phyAddr, PAGE4_4K, &pagCont);
	sq->SQE = (NVME_SUBMISSION_QUEUE_ENTRY *) (phyAddr | SYSTEM_LINEAR);
	memset(sq->SQE, 0, 4096);
	sq->COM.HAD = 0;
	sq->COM.TAL = 0;
	sq->COM.PHA = 0;
	return 0;
}
int NVMEWaitingRDY(NVME_CONTROLLER *ctrl, DWORD rdy)
{
	DWORD waitto = TimestampCPU() + ctrl->WTO;
	DWORD csts;
	while (rdy != ((csts = ctrl->CAP->CST) & NVME_CSTS_RDY))
	{
		__halt();
		if (csts & NVME_CSTS_FATAL)
		{
			OUTPUTTEXT("NVME FATAL ERROR DURING WAITING CONTROLLER READY\n");
			return -1;
		}
		if (TimestampCPU() > waitto)
		{
			OUTPUTTEXT("NVME CONTROLLER WAITING TIMEOUT\n");
			return -1;
		}
	}
	return 0;
}
NVME_COMPLETION_QUEUE_ENTRY NVMEWaitingCMD(NVME_SUBMISSION_QUEUE *sq, NVME_SUBMISSION_QUEUE_ENTRY *e)
{
	NVME_COMPLETION_QUEUE_ENTRY errcqe;
	memset(&errcqe, 0xFF, sizeof(NVME_COMPLETION_QUEUE_ENTRY));

	if (((sq->COM.HAD + 1) % (sq->COM.MSK + 1ULL)) == sq->COM.TAL)
	{
		OUTPUTTEXT("SUBMISSION QUEUE IS FULL\n");
		return errcqe;
	}

	// Commit
	NVME_SUBMISSION_QUEUE_ENTRY *sqe = sq->SQE + sq->COM.TAL;
	memcpy(sqe, e, sizeof(NVME_SUBMISSION_QUEUE_ENTRY));
	sqe->CDW0 |= sq->COM.TAL << 16;

	// Doorbell
	sq->COM.TAL++;
	sq->COM.TAL %= (sq->COM.MSK + 1ULL);
	sq->COM.DBL[0] = sq->COM.TAL;

	// Check completion
	NVME_COMPLETION_QUEUE *cq = sq->ICQ;
	DWORD timeout = TimestampCPU() + 5000; // sq->COM.CTR->WTO * 500;
	while ((cq->CQE[cq->COM.HAD].STS & 1) != cq->COM.PHA)
	{
		__halt();
		if (TimestampCPU() > timeout)
		{
			OUTPUTTEXT("NVME WAITING CMD TIMEOUT\n");
			return errcqe;
		}
	}

	// Consume CQE
	NVME_COMPLETION_QUEUE_ENTRY *cqe = cq->CQE + cq->COM.HAD;
	WORD cqNextHAD = (cq->COM.HAD + 1) % (cq->COM.MSK + 1ULL);
	if (cqNextHAD < cq->COM.HAD)
	{
		cq->COM.PHA ^= 1;
	}
	cq->COM.HAD = cqNextHAD;

	if (cqe->QHD != sq->COM.HAD)
	{
		sq->COM.HAD = cqe->QHD;
	}
	// Doorbell
	cq->COM.DBL[0] = cq->COM.HAD;
	return *cqe;
}
DWORD NVMETransfer(NVME_NAMESPACE *ns, void *buf, QWORD lba, DWORD count, DWORD write)
{
	if (!count) return 0;

	QWORD bufAddr = (QWORD) buf;
	DWORD maxCount = (4096 / ns->BSZ) - ((bufAddr & 0xFFF) / ns->BSZ);
	if (count > maxCount) count = maxCount;
	if (count > ns->MXRS) count = ns->MXRS;
	QWORD size = count * ns->BSZ;

	NVME_SUBMISSION_QUEUE_ENTRY sqe;
	memset(&sqe, 0, sizeof(NVME_SUBMISSION_QUEUE_ENTRY));
	sqe.CDW0 = write ? NVME_SQE_OPC_IO_WRITE : NVME_SQE_OPC_IO_READ;
	sqe.META = 0;
	sqe.DATA[0] = physical_mapping(bufAddr);
	sqe.DATA[1] = 0;
	sqe.NSID = ns->NSID;
	sqe.CDWA = lba;
	sqe.CDWB = lba >> 32;
	sqe.CDWC = (1UL << 31) | ((count - 1) & 0xFFFF);
	NVME_COMPLETION_QUEUE_ENTRY cqe = NVMEWaitingCMD(&ns->CTRL->ISQ, &sqe);
	if ((cqe.STS >> 1) & 0xFF)
	{
		OUTPUTTEXT("NVME CANNOT READ\n");
		return -1;
	}
	return count;
}
DWORD OperationNVME(DISK_OPERATION *op)
{
	NVME_NAMESPACE *ns = (NVME_NAMESPACE *) op->DRV;
	DWORD cc = DISK_RET_SUCCESS;
	QWORD phyAddr = 0;
	QWORD pagCont = 1;
	AllocatePhysicalMemory(&phyAddr, PAGE4_4K, &pagCont);
	BYTE *buf = (BYTE *) (phyAddr | SYSTEM_LINEAR);
	switch (op->CMD)
	{
		case CMD_IDENTIFY:
		{
			op->CNT = 1;
			memset(op->DAT, 0, sizeof(DISK_IDENTIFY));
			DISK_IDENTIFY *identify = (DISK_IDENTIFY *) op->DAT;
			memcpy(identify->SER, ns->CTRL->SER, sizeof(ns->CTRL->SER));
			memcpy(identify->MOD, ns->CTRL->MOD, sizeof(ns->CTRL->MOD));
			break;
		}
		case CMD_READ:
		case CMD_WRITE:
		{
			DWORD isWrite = op->CMD == CMD_WRITE;
			for (DWORD i = 0; i < op->CNT;)
			{
				DWORD remaining = op->CNT - i;
				if (remaining > (4096 / ns->BSZ)) remaining = (4096 / ns->BSZ);
				BYTE *opbuf = ((BYTE *) op->DAT) + (i * ns->BSZ);
				if (isWrite) memcpy(buf, opbuf, remaining * ns->BSZ);
				DWORD blocks = NVMETransfer(ns, buf, op->LBA + i, remaining, isWrite);
				if ((int) blocks < 0)
				{
					op->CNT = i;
					cc = DISK_RET_EBADTRACK;
					goto NVME_OVER;
				}
				if (!isWrite) memcpy(opbuf, buf, blocks * ns->BSZ);
				i += blocks;
			}
		}
		default: cc = DefaultDiskOperation(op);
	}
	NVME_OVER:;
	FreePhysicalMemory(phyAddr, PAGE4_4K, 1);
	return cc;
}
void ConfigureNVME(PCI_DEVICE *dvc)
{
	OutputPCIDevice(dvc->CMD);

	NVME_CAPABILITY *cap = (NVME_CAPABILITY *) PCIEnableMMIO(dvc->CMD, PCI_BASE_ADDRESS_0);
	if (!cap) return;

	if (!((cap->CAP >> 37) & 1))
	{
		OUTPUTTEXT("NVME CONTROLLER DOES NOT SUPPORT NVME COMMAND SET\n");
		return;
	}

	NVME_CONTROLLER *ctrl = (NVME_CONTROLLER *) HeapAlloc(HEAPK, sizeof(NVME_CONTROLLER));
	memset(ctrl, 0, sizeof(NVME_CONTROLLER));
	ctrl->DVC = dvc;
	ctrl->CAP = cap;
	ctrl->WTO = 500 * ((cap->CAP >> 24) & 0xFF);

	// Enable PCI BusMaster
	// Read command register and set MASTER bit
	__outdword(PCI_CONFIG_ADDRESS, dvc->CMD + PCI_COMMAND);
	WORD val = __inword(PCI_CONFIG_DATA) | PCI_COMMAND_MASTER;
	// Write to command register
	__outdword(PCI_CONFIG_ADDRESS, dvc->CMD + PCI_COMMAND);
	__outword(PCI_CONFIG_DATA, val);

	// RST controller
	ctrl->CAP->CC = 0;
	if (NVMEWaitingRDY(ctrl, 0))
	{
		OUTPUTTEXT("NVME FATAL ERROR DURING CONTROLLER SHUTDOWN\n");
		goto FAILED_NVME;
	}
	ctrl->DST = 4ULL << ((cap->CAP >> 32) & 0xF);

	int rc = NVMEConfigureCQ(ctrl, &ctrl->ACQ, 1, 4096 / sizeof(NVME_COMPLETION_QUEUE_ENTRY));
	if (rc)
	{
		goto FAILED_NVME;
	}

	rc = NVMEConfigureSQ(ctrl, &ctrl->ASQ, 0, 4096 / sizeof(NVME_SUBMISSION_QUEUE_ENTRY));
	if (rc)
	{
		goto FAILED_NVME;
	}
	ctrl->ASQ.ICQ = &ctrl->ACQ;

	ctrl->CAP->AQA = (ctrl->ACQ.COM.MSK << 16) | ctrl->ASQ.COM.MSK;
	ctrl->CAP->ASQ = physical_mapping((QWORD) ctrl->ASQ.SQE);
	ctrl->CAP->ACQ = physical_mapping((QWORD) ctrl->ACQ.CQE);

	ctrl->CAP->CC = 1 | (4 << 20) | (6 << 16);
	if (NVMEWaitingRDY(ctrl, 1))
	{
		OUTPUTTEXT("NVME FATAL ERROR DURING CONTROLLER ENABLING\n");
		goto FAILED_NVME;
	}

	/* The admin queue is set up and the controller is ready. Let's figure out
	   what namespaces we have. */
	// Identify Controller
	NVME_IDENTIFY_CONTROLLER *identify = 0;
	QWORD pagCont = 1;
	AllocatePhysicalMemory((QWORD *) &identify, PAGE4_4K, &pagCont);
	identify = (NVME_IDENTIFY_CONTROLLER *) (((QWORD) identify) | SYSTEM_LINEAR);
	memset(identify, 0, 4096);

	NVME_SUBMISSION_QUEUE_ENTRY sqe;
	memset(&sqe, 0, sizeof(NVME_SUBMISSION_QUEUE_ENTRY));
	sqe.CDW0 = NVME_SQE_OPC_ADMIN_IDENTIFY;
	sqe.META = 0;
	sqe.DATA[0] = physical_mapping((QWORD) identify);
	sqe.DATA[1] = 0;
	sqe.NSID = 0;
	sqe.CDWA = NVME_ADMIN_IDENTIFY_CNS_ID_CTRL;
	NVME_COMPLETION_QUEUE_ENTRY cqe = NVMEWaitingCMD(&ctrl->ASQ, &sqe);
	if ((cqe.STS >> 1) & 0xFF)
	{
		OUTPUTTEXT("CANNOT IDENTIFY NVME CONTROLLER\n");
		goto FAILED_NVME;
	}

	
	char buf[41];
	memcpy(buf, identify->SERN, sizeof(identify->SERN));
	buf[sizeof(identify->SERN)] = 0;
	char *serialN = LeadingWhitespace(buf, buf + sizeof(identify->SERN));
	memcpy(ctrl->SER, serialN, strlen(serialN));
	// OUTPUTTEXT(serialN);
	// LINEFEED();
	memcpy(buf, identify->MODN, sizeof(identify->MODN));
	buf[sizeof(identify->MODN)] = 0;
	serialN = LeadingWhitespace(buf, buf + sizeof(identify->MODN));
	memcpy(ctrl->MOD, serialN, strlen(serialN));
	// OUTPUTTEXT(serialN);
	// LINEFEED();
	

	ctrl->NSC = identify->NNAM;
	BYTE mdts = identify->MDTS;
	FreePhysicalMemory(physical_mapping((QWORD) identify), PAGE4_4K, 1);

	if (ctrl->NSC == 0)
	{
		OUTPUTTEXT("NO NAMESPACE\n");
		goto FAILED_NVME;
	}

	// Create I/O Queue
	// Create I/O CQ
	{
		DWORD qidx = 3;
		DWORD entryCount = 1 + (ctrl->CAP->CAP & 0xFFFF);
		if (entryCount > 4096 / sizeof(NVME_COMPLETION_QUEUE_ENTRY))
			entryCount = 4096 / sizeof(NVME_COMPLETION_QUEUE_ENTRY);
		if (NVMEConfigureCQ(ctrl, &ctrl->ICQ, qidx, entryCount))
		{
			OUTPUTTEXT("CANNOT INIT I/O CQ\n");
			goto FAILED_NVME;
		}
		NVME_SUBMISSION_QUEUE_ENTRY ccq;
		memset(&ccq, 0, sizeof(NVME_SUBMISSION_QUEUE_ENTRY));
		ccq.CDW0 = NVME_SQE_OPC_ADMIN_CREATE_IO_CQ;
		ccq.META = 0;
		ccq.DATA[0] = physical_mapping((QWORD) ctrl->ICQ.CQE);
		ccq.DATA[1] = 0;
		ccq.CDWA = (ctrl->ICQ.COM.MSK << 16) | (qidx >> 1);
		ccq.CDWB = 1;

		cqe = NVMEWaitingCMD(&ctrl->ASQ, &ccq);
		if ((cqe.STS >> 1) & 0xFF)
		{
			OUTPUTTEXT("CANNOT CREATE I/O CQ\n");
			goto FAILED_NVME;
		}
	}

	// Create I/O SQ
	{
		DWORD qidx = 2;
		DWORD entryCount = 1 + (ctrl->CAP->CAP & 0xFFFF);
		if (entryCount > 4096 / sizeof(NVME_SUBMISSION_QUEUE_ENTRY))
			entryCount = 4096 / sizeof(NVME_SUBMISSION_QUEUE_ENTRY);
		if (NVMEConfigureSQ(ctrl, &ctrl->ISQ, qidx, entryCount))
		{
			OUTPUTTEXT("CANNOT INIT I/O SQ\n");
			goto FAILED_NVME;
		}
		NVME_SUBMISSION_QUEUE_ENTRY csq;
		memset(&csq, 0, sizeof(NVME_SUBMISSION_QUEUE_ENTRY));
		csq.CDW0 = NVME_SQE_OPC_ADMIN_CREATE_IO_SQ;
		csq.META = 0;
		csq.DATA[0] = physical_mapping((QWORD) ctrl->ISQ.SQE);
		csq.DATA[1] = 0;
		csq.CDWA = (ctrl->ICQ.COM.MSK << 16) | (qidx >> 1);
		csq.CDWB = ((qidx >> 1) << 16) | 1;

		cqe = NVMEWaitingCMD(&ctrl->ASQ, &csq);
		if ((cqe.STS >> 1) & 0xFF)
		{
			OUTPUTTEXT("CANNOT CREATE I/O SQ\n");
			goto FAILED_NVME;
		}
		ctrl->ISQ.ICQ = &ctrl->ICQ;
	}

	/* Populate namespace IDs */
	for (DWORD nsidx = 0; nsidx < ctrl->NSC; nsidx++)
	{
		// Probe Namespace
		DWORD nsid = nsidx + 1;

		NVME_IDENTIFY_NAMESPACE *identifyNS = 0;
		pagCont = 1;
		AllocatePhysicalMemory((QWORD *) &identifyNS, PAGE4_4K, &pagCont);
		identifyNS = (NVME_IDENTIFY_NAMESPACE *) (((QWORD) identifyNS) | SYSTEM_LINEAR);
		memset(identifyNS, 0, 4096);

		memset(&sqe, 0, sizeof(NVME_SUBMISSION_QUEUE_ENTRY));
		sqe.CDW0 = NVME_SQE_OPC_ADMIN_IDENTIFY;
		sqe.META = 0;
		sqe.DATA[0] = physical_mapping((QWORD) identifyNS);
		sqe.DATA[1] = 0;
		sqe.NSID = nsid;
		sqe.CDWA = NVME_ADMIN_IDENTIFY_CNS_ID_NS;
		cqe = NVMEWaitingCMD(&ctrl->ASQ, &sqe);
		if ((cqe.STS >> 1) & 0xFF)
		{
			OUTPUTTEXT("CANNOT IDENTIFY NAMESPACE ");
			PRINTRAX(nsid, 4);
			LINEFEED();
			goto FAILED_NAMESPACE;
		}

		BYTE currentLBAFormat = identifyNS->FLBA & 0xF;
		if (currentLBAFormat > identifyNS->NLBA)
		{
			OUTPUTTEXT("NVME NAMESPACE ");
			PRINTRAX(nsid, 4);
			OUTPUTTEXT(" CURRENT LBA FORMAT ");
			PRINTRAX(currentLBAFormat, 1);
			OUTPUTTEXT(" IS BEYOND WHAT THE NAMESPACE SUPPORTS ");
			PRINTRAX(identifyNS->NLBA + 1, 2);
			LINEFEED();
			goto FAILED_NAMESPACE;
		}

		if (!identifyNS->SIZE)
		{
			goto FAILED_NAMESPACE;
		}

		if (!NVME_DMA_MEMORY)
		{
			pagCont = 1;
			AllocatePhysicalMemory((QWORD *) &NVME_DMA_MEMORY, PAGE4_4K, &pagCont);
		}

		NVME_NAMESPACE *ns = HeapAlloc(HEAPK, sizeof(NVME_NAMESPACE));
		memset(ns, 0, sizeof(NVME_NAMESPACE));
		ns->CTRL = ctrl;
		ns->NSID = nsid;
		ns->NLBA = identifyNS->SIZE;

		NVME_LOGICAL_BLOCK_ADDRESS *fmt = identifyNS->LBAF + currentLBAFormat;

		ns->BSZ = 1ULL << fmt->DS;
		ns->META = fmt->MS;
		if (ns->BSZ > 4096)
		{
			OUTPUTTEXT("NVME NAMESPACE ");
			PRINTRAX(nsid, 4);
			OUTPUTTEXT(" BLOCK SIZE ");
			PRINTRAX(ns->BSZ, 4);
			OUTPUTTEXT(" > 1000");
			HeapFree(ns);
			goto FAILED_NAMESPACE;
		}
		/*
		OUTPUTTEXT("NVME NAMESPACE ");
		PRINTRAX(nsid, 4);
		OUTPUTTEXT(" BLOCK SIZE ");
		PRINTRAX(ns->BSZ, 8);
		OUTPUTTEXT(" LBA COUNT ");
		PRINTRAX(identifyNS->SIZE, 16);
		LINEFEED();
		*/

		ns->DRV.ID = nsidx;
		ns->DRV.DVR.RMV = 0;
		ns->DRV.DVR.TYPE = DTYPE_NVME;
		ns->DRV.BS = ns->BSZ;
		ns->DRV.SCT = ns->NLBA;
		ns->DRV.OP = OperationNVME;

		if (mdts)
		{
			ns->MXRS = ((1ULL << mdts) * 4096) / ns->BSZ;
		}
		else
		{
			ns->MXRS = -1;
		}
		LinkupDisk((DISK_DRIVER *) ns);

		/*
		memset(identifyNS, 0, 4096);
		DISK_OPERATION dop;
		memset(&dop, 0, sizeof(DISK_OPERATION));
		dop.DRV = (DISK_DRIVER *) ns;
		dop.BSZ = ns->BSZ;
		dop.CMD = CMD_IDENTIFY;
		dop.LBA = 0;
		dop.CNT = 2;
		dop.DAT = identifyNS;
		ExecuteDiskOperation(&dop);
		DISK_IDENTIFY *did = (DISK_IDENTIFY *) identifyNS;
		OUTPUTTEXT(did->SER);
		LINEFEED();
		OUTPUTTEXT(did->MOD);
		LINEFEED();
		*/

		FAILED_NAMESPACE:;
		FreePhysicalMemory(physical_mapping((QWORD) identifyNS), PAGE4_4K, 1);
	}



	return;
	FAILED_NVME:;
	if (ctrl->ICQ.CQE)
	{
		FreePhysicalMemory(physical_mapping((QWORD) ctrl->ICQ.CQE), PAGE4_4K, 1);
	}
	if (ctrl->ISQ.SQE)
	{
		FreePhysicalMemory(physical_mapping((QWORD) ctrl->ISQ.SQE), PAGE4_4K, 1);
	}
	if (ctrl->ACQ.CQE)
	{
		FreePhysicalMemory(physical_mapping((QWORD) ctrl->ACQ.CQE), PAGE4_4K, 1);
	}
	if (ctrl->ASQ.SQE)
	{
		FreePhysicalMemory(physical_mapping((QWORD) ctrl->ASQ.SQE), PAGE4_4K, 1);
	}
	HeapFree(ctrl);
}
