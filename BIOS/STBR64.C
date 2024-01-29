#define PCI_COMMAND         0x04	/* 16 bits */
#define PCI_BASE_ADDRESS_5	0x24	/* 32 bits */

#define PCI_COMMAND_MEMORY  0x2	/* Enable response in Memory space */
#define PCI_COMMAND_MASTER  0x4	/* Enable bus mastering */

#define HOST_CTL_AHCI_EN    (1 << 31) /* AHCI enabled */

#define HBA_PORT_IPM_ACTIVE  1
#define HBA_PORT_DET_PRESENT 3

#define HBA_PORT_CMD_ST      0x0001
#define HBA_PORT_CMD_SPIN_UP 0x0002
#define HBA_PORT_CMD_FRE     0x0010
#define HBA_PORT_CMD_FR      0x4000
#define HBA_PORT_CMD_CR      0x8000

#define	SATA_SIG_ATA    0x00000101  // SATA drive
#define	SATA_SIG_ATAPI  0xEB140101  // SATAPI drive
#define	SATA_SIG_SEMB   0xC33C0101  // Enclosure management bridge
#define	SATA_SIG_PM     0x96690101  // Port multiplier 

#define AHCI_DEVICE_NULL   0
#define AHCI_DEVICE_SATA   1
#define AHCI_DEVICE_SEMB   2
#define AHCI_DEVICE_PM     3
#define AHCI_DEVICE_SATAPI 4

#define ATA_CMD_READ_DMA_EX                 0x25

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ  0x08

#define HBA_PxIS_TFES   (1 << 30)       /* TFES - Task File Error Status */


typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned long long QWORD;


typedef volatile struct _HBA_PORT
{
	DWORD clb;		// 0x00, command list base address, 1K-byte aligned
	DWORD clbu;		// 0x04, command list base address upper 32 bits
	DWORD fb;		// 0x08, FIS base address, 256-byte aligned
	DWORD fbu;		// 0x0C, FIS base address upper 32 bits
	DWORD is;		// 0x10, interrupt status
	DWORD ie;		// 0x14, interrupt enable
	DWORD cmd;		// 0x18, command and status
	DWORD rsv0;		// 0x1C, Reserved
	DWORD tfd;		// 0x20, task file data
	DWORD sig;		// 0x24, signature
	DWORD ssts;		// 0x28, SATA status (SCR0:SStatus)
	DWORD sctl;		// 0x2C, SATA control (SCR2:SControl)
	DWORD serr;		// 0x30, SATA error (SCR1:SError)
	DWORD sact;		// 0x34, SATA active (SCR3:SActive)
	DWORD ci;		// 0x38, command issue
	DWORD sntf;		// 0x3C, SATA notification (SCR4:SNotification)
	DWORD fbs;		// 0x40, FIS-based switch control
	DWORD rsv1[11];	// 0x44 ~ 0x6F, Reserved
	DWORD vendor[4];// 0x70 ~ 0x7F, vendor specific
} HBA_PORT;
typedef volatile struct _HBA_MEM
{
	// 0x00 - 0x2B, Generic Host Control
	DWORD cap;     // 0x00, Host capability
	DWORD ghc;     // 0x04, Global host control
	DWORD is;      // 0x08, Interrupt status
	DWORD pi;      // 0x0C, Port implemented
	DWORD vs;      // 0x10, Version
	DWORD ccc_ctl; // 0x14, Command completion coalescing control
	DWORD ccc_pts; // 0x18, Command completion coalescing ports
	DWORD em_loc;  // 0x1C, Enclosure management location
	DWORD em_ctl;  // 0x20, Enclosure management control
	DWORD cap2;    // 0x24, Host capabilities extended
	DWORD bohc;    // 0x28, BIOS/OS handoff control and status

	// 0x2C - 0x9F, Reserved
	BYTE rsv[0xA0 - 0x2C];

	// 0xA0 - 0xFF, Vendor specific registers
	BYTE vendor[0x100 - 0x0A0];

	// 0x100 - 0x10FF, Port control registers
	HBA_PORT ports[];
} HBA_MEM;
typedef struct _HBA_CMD_HEADER
{
	// DW0
	BYTE  cfl : 5;        // Command FIS length in DWORDS, 2 ~ 16
	BYTE  a : 1;          // ATAPI
	BYTE  w : 1;          // Write, 1: H2D, 0: D2H
	BYTE  p : 1;          // Prefetchable

	BYTE  r : 1;          // Reset
	BYTE  b : 1;          // BIST
	BYTE  c : 1;          // Clear busy upon R_OK
	BYTE  rsv0 : 1;       // Reserved
	BYTE  pmp : 4;        // Port multiplier port

	WORD prdtl;           // Physical region descriptor table length in entries

	// DW1
	volatile DWORD prdbc; // Physical region descriptor byte count transferred

	// DW2, 3
	DWORD ctba;           // Command table descriptor base address
	DWORD ctbau;          // Command table descriptor base address upper 32 bits

	// DW4 - 7
	DWORD rsv1[4];        // Reserved
} HBA_CMD_HEADER;
typedef struct _HBA_PRDT_ENTRY
{
	DWORD dba;		// Data base address
	DWORD dbau;		// Data base address upper 32 bits
	DWORD rsv0;		// Reserved

	// DW3
	DWORD dbc : 22;		// Byte count, 4M max
	DWORD rsv1 : 9;		// Reserved
	DWORD i : 1;		// Interrupt on completion
} HBA_PRDT_ENTRY;
typedef struct _HBA_CMD_TBL
{
	// 0x00
	BYTE cfis[64]; // Command FIS

	// 0x40
	BYTE  acmd[16];   // ATAPI command, 12 or 16 bytes

	// 0x50
	BYTE  rsv[48];    // Reserved

	// 0x80
	HBA_PRDT_ENTRY	prdt_entry[];	// Physical region descriptor table entries, 0 ~ 65535
} HBA_CMD_TBL;
typedef struct _FIS_REG_H2D
{
	// DWORD 0
	BYTE  fis_type;   // FIS_TYPE_REG_H2D

	BYTE  pmport : 4; // Port multiplier
	BYTE  rsv0 : 3;   // Reserved
	BYTE  c : 1;      // 1: Command, 0: Control

	BYTE  command;    // Command register
	BYTE  featurel;   // Feature register, 7:0

	// DWORD 1
	BYTE  lba0;       // LBA low register, 7:0
	BYTE  lba1;       // LBA mid register, 15:8
	BYTE  lba2;       // LBA high register, 23:16
	BYTE  device;     // Device register

	// DWORD 2
	BYTE  lba3;       // LBA register, 31:24
	BYTE  lba4;       // LBA register, 39:32
	BYTE  lba5;       // LBA register, 47:40
	BYTE  featureh;   // Feature register, 15:8

	// DWORD 3
	BYTE  countl;     // Count register, 7:0
	BYTE  counth;     // Count register, 15:8
	BYTE  icc;        // Isochronous command completion
	BYTE  control;    // Control register

	// DWORD 4
	BYTE res_2[64 - 16];
} FIS_REG_H2D;
typedef struct _NTFS_BPB
{
	BYTE opcode[3];
	BYTE OEM[8];
	BYTE BPS[2];   // byte per sector, always 512
	BYTE cluster;  // sector per cluster
	BYTE reserved1[7];
	BYTE descriptor;
	BYTE reserved2[2];
	WORD track;    // sector per track
	WORD head;
	DWORD hidden;  // hidden sectors
	BYTE reserved3[8];
	QWORD sector;  // sector count
	QWORD MFT;     // $MFT sector
	QWORD MFTMIRR; // $MFTMirr sector
	BYTE FILE;     // FILE size
	BYTE reserved4[3];
	BYTE INDX;     // INDX size
	BYTE reserved5[3];
	QWORD serial;
	DWORD checksum;
} NTFS_BPB;


void* __cdecl memset(void*, int, unsigned long long);
void* __cdecl memcpy(void*, const void*, unsigned long long);
void SCROLLPAGE();
void MOVECURSOR();
void CARRIAGERETURN();
void LINEFEED();
void OUTPUTTEXT(const char *);
void PRINTRAX(QWORD, BYTE);
DWORD find_pci();
DWORD pci_device(DWORD, DWORD);
DWORD ahci_controller_setup(DWORD);
void* pci_enable_mmio(DWORD, DWORD, DWORD);
DWORD check_type(HBA_PORT*);
DWORD port_rebase(HBA_PORT*);
DWORD LoadingSATA(HBA_PORT*);
void stop_cmd(HBA_PORT*);

/*
* Cursor pos in 80x25 mode
*/
DWORD CURSOR = 0;
// DISK GUID = {BE559BDA-5715-4BAB-89D3-66D22BF6A8B6}
BYTE GUID0[] = { 0xDA, 0x9B, 0x55, 0xBE, 0x15, 0x57, 0xAB, 0x4B, 0x89, 0xD3, 0x66, 0xD2, 0x2B, 0xF6, 0xA8, 0xB6 };
// PART GUID = {986AFD81-09FF-4490-AED6-C7597A5AA827}
BYTE GUID1[] = { 0x81, 0xFD, 0x6A, 0x98, 0xFF, 0x09, 0x90, 0x44, 0xAE, 0xD6, 0xC7, 0x59, 0x7A, 0x5A, 0xA8, 0x27 };
// KERNEL.DLL MFT RECORD
const DWORD MFT_RECORD = 0x30;

void mainCRTStartup()
{
	OUTPUTTEXT("STRAWBERRY OS\n");
	if (!find_pci())
	{
		// DOS HEADER
		QWORD base = 0xFFFF800000000000ULL;
		// NT HEADER = ImageBase + [DOS_HEADER + 0x3C]
		QWORD NTHeader = base + *((DWORD *) (base + 0x3C));
		// [NT_HEADER + 0x30] is the absolute liner address of image base
		*((QWORD *) (NTHeader + 0x30)) = base;
		// ImageBase + [NT_HEADER + 0x28] is the address of entry point
		((void (*)()) (base + *((DWORD *)(NTHeader + 0x28))))();
	}
	else while (1);
}
void MOVECURSOR()
{
	if (CURSOR >= 0x07D0)
	{
		CURSOR -= 0x50;
		SCROLLPAGE();
	}
	// 0x3D5:0x0E high 8 bit
	__outbyte(0x03D4, 0x0E);
	__outbyte(0x03D5, CURSOR >> 8);
	// 0x3D5:0x0F low 8 bit
	__outbyte(0x03D4, 0x0F);
	__outbyte(0x03D5, CURSOR & 0xFF);
}
void CARRIAGERETURN()
{
	CURSOR -= (CURSOR % 0x50);
	MOVECURSOR();
}
void LINEFEED()
{
	// \r
	CARRIAGERETURN();
	// \n:move cursor to next line: cursor + 0x50
	CURSOR += 0x50;
	MOVECURSOR();
}
void OUTPUTTEXT(const char* s)
{
	while (*s)
	{
		// low 8 bit is ASCII code
		WORD x = *s;
		if (x == '\r')
		{
			CARRIAGERETURN();
		}
		else if (x == '\n')
		{
			LINEFEED();
		}
		else
		{
			// not CR or LF, output ascii code
			// high 8 bit is text attribute
			x |= 0x700;
			// write to cursor position
			*((WORD *) (0x000B8000ULL + ((QWORD) CURSOR << 1))) = x;
			// Increment cursor pos
			CURSOR++;
		}
		s++;
		MOVECURSOR();
	}
}
const char HEXDIG[] = "0123456789ABCDEF";
void PRINTRAX(QWORD x, BYTE s)
{
	char buf[17];
	buf[s] = 0;
	while (s--)
	{
		buf[s] = HEXDIG[x & 0xF];
		x >>= 4;
	}
	OUTPUTTEXT(buf);
}
DWORD find_pci()
{
	// Config Address
	DWORD cmd = 0x80000000;
	while (cmd < 0x81000000)
	{
		__outdword(0x0CF8, cmd);
		DWORD id = __indword(0x0CFC);
		WORD vendor = id & 0xFFFF;
		// Check wether device is valid
		if (vendor && vendor != 0xFFFF)
		{
			if (!pci_device(cmd, id))
			{
				return 0;
			}
		}
		cmd += 0x800;
	}
	return 1;
}
DWORD pci_device(DWORD cmd, DWORD id)
{
	// Read class code
	__outdword(0x0CF8, cmd + 0x08);
	DWORD class = (__indword(0x0CFC) >> 8);

	if (class == 0x010601) // AHCI Controller
	{
		return ahci_controller_setup(cmd);
	}

	return 1;
}
DWORD ahci_controller_setup(DWORD dvc)
{
	// Config MMIO
	void* iobase = pci_enable_mmio(dvc, PCI_BASE_ADDRESS_5, 0x13000);
	if (!iobase)
		return 1;

	// Enable busmaster
	// Read Command register and set MASTER bit
	__outdword(0x0CF8, dvc + PCI_COMMAND);
	WORD val = __inword(0x0CFC) | PCI_COMMAND_MASTER;
	// Write to Command register
	__outdword(0x0CF8, dvc + PCI_COMMAND);
	__outword(0x0CFC, val);

	HBA_MEM *abar = (HBA_MEM *) iobase;
	abar->ghc |= HOST_CTL_AHCI_EN;

	// State, 0 means success
	DWORD find = 1;
	// Port status, 1 means usable
	DWORD pi = abar->pi;
	// AHCI Port
	HBA_PORT *port = abar->ports;
	while (pi && find)
	{
		if (pi & 1)
		{
			if (check_type(port) == AHCI_DEVICE_SATA)
			{
				// Try to read kernel
				if (!port_rebase(port))
				{
					find &= LoadingSATA(port);
				}
				stop_cmd(port);
			}
		}
		// Next port
		pi >>= 1;
		port++;
	}

	return find;
}
void* pci_enable_mmio(DWORD device, DWORD addr, DWORD base)
{
	// Read BAR
	__outdword(0x0CF8, device + addr);
	DWORD bar = __indword(0x0CFC);
	// Check BAR PIO Mode
	if (bar & 1)
	{
		OUTPUTTEXT("ERR:PIO ENABLED\n");
		return 0;
	}
	// Use base as mmio address, write to BAR
	__outdword(0x0CF8, device + addr);
	__outdword(0x0CFC, base | (bar & 0xF));
	// Read Command register
	__outdword(0x0CF8, device + PCI_COMMAND);
	WORD cmd = __inword(0x0CFC);
	// Set Memory Space Enable bit
	cmd |= PCI_COMMAND_MEMORY;
	// Write to command register
	__outdword(0x0CF8, device + PCI_COMMAND);
	__outword(0x0CFC, cmd);
	return (void*)(QWORD)(base);
}
DWORD check_type(HBA_PORT* port)
{
	DWORD ssts = port->ssts;

	BYTE ipm = (ssts >> 8) & 0x0F;
	BYTE det = (ssts >> 0) & 0x0F;

	// Check device present
	if (det != HBA_PORT_DET_PRESENT)
		return AHCI_DEVICE_NULL;
	// Check device active
	if (ipm != HBA_PORT_IPM_ACTIVE)
		return AHCI_DEVICE_NULL;

	switch (port->sig)
	{
		case SATA_SIG_ATA:   return AHCI_DEVICE_SATA;
		case SATA_SIG_ATAPI: return AHCI_DEVICE_SATAPI;
		case SATA_SIG_SEMB:  return AHCI_DEVICE_SEMB;
		case SATA_SIG_PM:    return AHCI_DEVICE_PM;
		default:             return AHCI_DEVICE_NULL;
	}
}
// Stop command engine
void stop_cmd(HBA_PORT* port)
{
	// Clear ST (bit 0)
	port->cmd &= ~HBA_PORT_CMD_ST;

	// Clear FRE (bit4);
	port->cmd &= ~HBA_PORT_CMD_FRE;

	// Wait until FR (bit14), CR (bit15) are cleared;
	while (port->cmd & (HBA_PORT_CMD_FR | HBA_PORT_CMD_CR));
}
DWORD port_rebase(HBA_PORT* port)
{
	// Use 3 pages
	QWORD addr = 0x00015000;
	// Stop command engine
	stop_cmd(port);

	// Command list offset: ADDR 1KiB aligned
	// Command list entry size = 32
	// Command list entry maxim count  = 32
	// Command list maxim size = 32*32 = 1024 per port
	port->clb = addr;
	port->clbu = addr >> 32;
	memset((void*)(port->clb | ((QWORD)port->clbu << 32)), 0, 1024);

	// FIS offset base: ADDR+9KiB 256B aligned
	// FIS entry size = 256B per port
	port->fb = (addr + 9 * 1024);
	port->fbu = (addr + 9 * 1024) >> 32;
	memset((void*)(port->fb | ((QWORD)port->fbu << 32)), 0, 256);

	// Command table offset: ADDR+1KiB
	// Command table size = 256*32 = 8KiB per port
	HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*)(port->clb | ((QWORD)port->clbu << 32));
	for (int i = 0; i < 32; i++)
	{
		cmdheader[i].prdtl = 8; // 8 prdt entries per command table
		// 256B per command table, 64+16+48+16*8
		// Command table offset; ADDR+1KB+i*256B
		cmdheader[i].ctba = (addr + 1024 + i * 256);
		cmdheader[i].ctbau = (addr + 1024 + i * 256) >> 32;
		memset((void*)(cmdheader[i].ctba | ((QWORD)cmdheader[i].ctbau << 32)), 0, 256);
	}

	// Start command engine
	// Wait until CR (bit15) is cleared
	while (port->cmd & HBA_PORT_CMD_CR);

	// Set FRE (bit4) and ST (bit0)
	DWORD cmd = port->cmd;
	cmd |= HBA_PORT_CMD_FRE;
	cmd |= HBA_PORT_CMD_ST;
	cmd |= HBA_PORT_CMD_SPIN_UP;
	port->cmd = cmd;
	int spin = 0;
	while (spin++ < 1000000)
	{
		if ((port->ssts & 7) == 3)
		{
			// Success
			return 0;
		}
	}
	OUTPUTTEXT("AHCI LINK DOWN\n");
	return 2;
}
DWORD AHCIIO(HBA_PORT* port, QWORD sector, WORD count, void* buffer)
{
	// Clear pending interrupt bits
	port->is = -1;
	int slot = 0;// ahci_find_cmdslot(port);
	if (slot == -1)
		return 2;

	HBA_CMD_HEADER* cmdh = (HBA_CMD_HEADER*)(port->clb | ((QWORD)port->clbu << 32));
	cmdh += slot;
	// Command FIS size
	cmdh->cfl = 5; // sizeof(FIS_REG_H2D) / sizeof(DWORD)
	// Read from device
	cmdh->w = 0;
	// cmdh->c = 1;
	// cmdh->p = 1;
	// PRDT entries count
	cmdh->prdtl = ((count - 1) >> 4) + 1; // UPPER BOUND (count / 4)

	HBA_CMD_TBL* tbl = (HBA_CMD_TBL*)(cmdh->ctba | ((QWORD)cmdh->ctbau << 32));
	memset(tbl, 0, sizeof(HBA_CMD_TBL) + (cmdh->prdtl * sizeof(HBA_PRDT_ENTRY)));
	// 8KiB (16 sectors) per PRDT
	QWORD buf = (QWORD)buffer;
	WORD count1 = count;
	DWORD i = 0;
	while (count1)
	{
		DWORD cnt = (count1 < 16) ? count1 : 16;
		tbl->prdt_entry[i].dba = (DWORD)buf;
		tbl->prdt_entry[i].dbau = buf >> 32;
		tbl->prdt_entry[i].dbc = (cnt << 9) - 1;
		tbl->prdt_entry[i].i = 0; // Right or Wrong ?
		count1 -= cnt; // 16 sectors
		buf += ((QWORD)cnt << 9); // 8KiB
		i++;
	}

	// Setup command
	FIS_REG_H2D* fis = (FIS_REG_H2D*)tbl->cfis;
	memset(fis, 0, sizeof(FIS_REG_H2D));

	fis->fis_type = 0x27; // Register FIS - host to device
	fis->c = 1; // Command
	fis->command = ATA_CMD_READ_DMA_EX;
	fis->featurel = 1; /* dma */

	fis->lba0 = (sector >> 0) & 0xFF;
	fis->lba1 = (sector >> 8) & 0xFF;
	fis->lba2 = (sector >> 16) & 0xFF;
	fis->device = 1 << 6; // LBA MODE
	fis->lba3 = (sector >> 24) & 0xFF;
	fis->lba4 = (sector >> 32) & 0xFF;
	fis->lba5 = (sector >> 40) & 0xFF;

	fis->countl = (count >> 0) & 0xFF;
	fis->counth = (count >> 8) & 0xFF;

	// The below loop waits until the port is no longer busy bufore issuing a new command
	int spin = 0; // Spin lock timeout counter
	while ((port->tfd & (ATA_DEV_DRQ | ATA_DEV_BUSY)) && spin < 1000000)
	{
		spin++;
	}
	if (spin == 1000000)
	{
		return 3;
	}

	port->ci = 1 << slot;

	// Wait for completion
	while ((port->ci & (1 << slot)) && !(port->is & HBA_PxIS_TFES));
	if (port->is & HBA_PxIS_TFES)
	{
		return 4;
	}
	return 0;
}
DWORD READVD(BYTE* addr, BYTE size)
{
	QWORD x = 0;
	memcpy(&x, addr, size);
	BYTE bw = size << 3;
	if (x & (1ULL << (bw - 1)))
	{
		x |= ((0xFFFFFFFFFFFFFFFFULL >> bw) << bw);
	}
	return x;
}
DWORD LoadingSATA(HBA_PORT* port)
{
	// Use 1 page as buffer
	QWORD page = 0x00018000;
	// Read LBA 1: GPT Header
	if (AHCIIO(port, 1, 1, (void*)page))
	{
		OUTPUTTEXT("DISK READ ERROR\n");
		return 1;
	}

	// Assume GPT and check DISK GUID
	QWORD* guid = (QWORD*)(page + 0x38);
	if (guid[0] != ((QWORD*)GUID0)[0] && guid[1] != ((QWORD*)GUID0)[1])
	{
		return 1;
	}

	// FIND NTFS SYSTEM PART
	// Read partition table
	if (AHCIIO(port, *((QWORD*)(page + 0x48)), 1, (void*)page))
	{
		OUTPUTTEXT("DISK READ ERROR\n");
		return 1;
	}
	memset((void*)(page + 0x200), 0, 0x200);
	QWORD partEntry = page;
	for (; partEntry < page + 0x200; partEntry += 0x80)
	{
		QWORD* partGUID = (QWORD*)(partEntry + 0x10);
		if (!(partGUID[0] | partGUID[1]))
			break;

		if (partGUID[0] == ((QWORD*)GUID1)[0] && partGUID[1] == ((QWORD*)GUID1)[1])
			break;
	}
	if (!(((QWORD*)(partEntry + 0x10))[0] | ((QWORD*)(partEntry + 0x10))[1]))
	{
		OUTPUTTEXT("NO NTFS SYSTEM PART\n");
		return 1;
	}

	// Partition start sector LBA
	QWORD partiionSector = *((QWORD *) (partEntry + 0x20));
	// Read partition first sector
	if (AHCIIO(port, partiionSector, 1, (void*)page))
	{
		OUTPUTTEXT("DISK READ ERROR\n");
		return 1;
	}
	NTFS_BPB BPB;
	memcpy(&BPB, (void*)page, sizeof(NTFS_BPB));

	// $MFT LBA
	QWORD mftsector = BPB.hidden + (BPB.MFT * BPB.cluster);
	// Read 2 sector file record of KERNEL.DLL file
	if (AHCIIO(port, mftsector + ((QWORD) MFT_RECORD << 1), 2, (void*)page))
	{
		OUTPUTTEXT("DISK READ ERROR\n");
		return 1;
	}
	// Fix the end of the sector by update array
	*((WORD*)(page + 0x03FE)) = *((WORD*)(page + 0x34));
	*((WORD*)(page + 0x01FE)) = *((WORD*)(page + 0x32));
	// First attribute
	BYTE* attribute = (BYTE*)(page);
	attribute += *((WORD*)(attribute + 0x14));
	// Find attribute 0x80
	while (*((WORD*)attribute) != 0xFFFF && *((WORD*)attribute) != 0x0080)
	{
		attribute += *((DWORD*)(attribute + 4));
	}
	// Not found
	if (*((WORD*)attribute) == 0xFFFF)
	{
		OUTPUTTEXT("NO 80 DATA RUN LIST\n");
		return 1;
	}

	// Resolve run list
	attribute += *((WORD*)(attribute + 0x20));
	// Previous run list LBA
	QWORD RUNLIST = BPB.hidden;
	// Physical address of dest, kernel area
	BYTE* dst = (BYTE *) 0x00200000ULL;
	while (*attribute)
	{
		// Compressed byte
		BYTE compressed = *attribute;
		// High 4 bit is data's cluster number
		DWORD startsize = compressed >> 4;
		// Low 4 bit is cluster count
		DWORD clustersize = compressed & 0xF;
		attribute++;
		// Read cluster count
		QWORD cluster = READVD(attribute, clustersize);
		attribute += clustersize;
		// Read data cluster, add to run list LBA
		RUNLIST += READVD(attribute, startsize) * BPB.cluster;
		attribute += startsize;
		// Read clusters
		QWORD sector = RUNLIST;
		while (cluster--)
		{
			if (AHCIIO(port, sector, BPB.cluster, dst))
			{
				OUTPUTTEXT("DISK READ ERROR\n");
				return 1;
			}

			sector += BPB.cluster;
			dst += ((QWORD)BPB.cluster) << 9;
		}
	}
	return 0;
}