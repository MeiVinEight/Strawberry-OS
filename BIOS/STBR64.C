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
#define ATA_CMD_IDENTIFY_DEVICE             0xEC

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ  0x08

#define HBA_PxIS_TFES   (1 << 30)       /* TFES - Task File Error Status */


#define SYSTEM_LNR 0xFFFF800000000000ULL
#define KERNEL_LNR 0xFFFFFFFFFF000000ULL

#define SYSTEM_TABLE_MMAP 0x00003018ULL


#define SECTION "TEXT"
#define BIOSAPI __declspec(allocate(SECTION))
#pragma section(SECTION, read, write)


typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned long long QWORD;


typedef struct _STBRBOOT
{
	BYTE JMP[8];
	QWORD MFT;
	QWORD GUID0[2];
	QWORD GUID1[2];
	WORD LONGMODE;
	BYTE DM;
	BYTE CM;
	WORD H;
	WORD V;
	QWORD A0;
	QWORD RSDP;
	BYTE CODE[];
} STBRBOOT;
typedef struct _CONSOLE_SCREEN
{
	QWORD A0;
	QWORD A1;
	WORD  H;
	WORD  V;
	WORD  ROW;
	WORD  CLM;
	DWORD CSR;
	BYTE  DM;
	BYTE  CM;
	BYTE  CLR;
} CONSOLE_SCREEN;
typedef struct _TEXT_MODE_MEMORY
{
	BYTE *TEXT;   // Data address in text mode
	DWORD CURSOR; // Current cursor position
} TEXT_MODE_MEMORY;
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
typedef struct _OS_SYSTEM_TABLE
{
	QWORD FONT;
	QWORD MMAP;
	QWORD BLAT;
	QWORD SCRN;
	QWORD RSDP;
} OS_SYSTEM_TABLE;


void* __cdecl memset(void*, int, unsigned long long);
void* __cdecl memcpy(void*, const void*, unsigned long long);
void PAINTCURSOR(DWORD);
void PAINTCHAR(BYTE, BYTE, DWORD);
void SCROLLPAGE();
void MOVECURSOR();
void CARRIAGERETURN();
void LINEFEED();
void OUTCHAR(char);
void OUTPUTTEXT(const char *);
DWORD find_pci();
DWORD pci_device(DWORD, DWORD);
DWORD ahci_controller_setup(DWORD);
void* pci_enable_mmio(DWORD, DWORD);
DWORD check_type(HBA_PORT*);
DWORD start_cmd(HBA_PORT*);
DWORD AHCIIO(HBA_PORT *, QWORD, WORD, void *, DWORD);
DWORD LoadingSATA(HBA_PORT*, QWORD);
void stop_cmd(HBA_PORT*);

/*
* Cursor pos in 80x25 mode
*/
BIOSAPI TEXT_MODE_MEMORY TEXT_MODE;
BIOSAPI CONSOLE_SCREEN SCREEN;
BIOSAPI DWORD COLOR_PLAETTE[] =
{
	0x000000,
	0x0000AA,
	0x00AA00,
	0x00AAAA,
	0xAA0000,
	0xAA00AA,
	0xFFAA00,
	0xAAAAAA,
	0x555555,
	0x5555FF,
	0x55FF55,
	0x55FFFF,
	0xFF5555,
	0xFF55FF,
	0xFFFF55,
	0xFFFFFF
};
BIOSAPI const char ERR00[] = "ERR:PIO ENABLED\n";
BIOSAPI const char ERR01[] = "AHCI LINK DOWN\n";
BIOSAPI const char ERR02[] = "DISK READ ERROR\n";
BIOSAPI const char ERR03[] = "NO NTFS SYSTEM PART\n";
BIOSAPI const char ERR04[] = "NO 80 DATA RUN LIST\n";
BIOSAPI const char ERR05[] = "NO KERNEL.DLL\n";
BIOSAPI const char ERR06[] = "NO EMPTY PAGE\n";
BIOSAPI const char MSG0000[] = "Strawberry-BOOT\n";
BIOSAPI const char MSG0001[] = "PCI:FIND AHCI CONTROLLER\n";
BIOSAPI const char MSG0002[] = "FIND BIOS RSDP\n";
BIOSAPI const char MSG0003[] = "KERNEL ENTRY ";
BIOSAPI const char DVC07E015AD[] = "VMware SATA AHCI controller";
BIOSAPI const char DVC06D38086[] = "Intel(R) 400 Series Chipset Family SATA AHCI Controller";
BIOSAPI const char DVCA2828086[] = "Intel Corporation 200 Series PCH SATA controller [AHCI mode]";
BIOSAPI STBRBOOT *BOOT_TABLE = (STBRBOOT *) (0x00010000 | SYSTEM_LNR);
BIOSAPI OS_SYSTEM_TABLE SYSTEM_TABLE;

void allocate_physical(QWORD *physicalAddress, QWORD pageSize, QWORD *pageCount)
{
	// Foreach the memory map
	QWORD *mmap = (QWORD *) SYSTEM_TABLE_MMAP;
	QWORD *end = (QWORD *) mmap[0];
	QWORD *beg = mmap + 1;
	// Calculate page shift
	QWORD shift = 12 + 9 * pageSize;

	while (beg < end)
	{
		QWORD A0 = beg[0];
		QWORD S0 = beg[1];
		QWORD F0 = beg[2];
		if ((DWORD) F0 != 1)
		{
			goto CONTINUE;
		}
		// Skip 0x00000000 + 1M
		if (A0 < (1 << 20))
		{
			goto CONTINUE;
		}
		// Align A0 and S0 to the first boundary of page size
		QWORD A1 = A0;
		// Calculate align gap size
		QWORD S1 = A0 & ((1ULL << shift) - 1);
		S1 = (-S1) & ((1ULL << shift) - 1);
		if (S0 < S1)
		{
			goto CONTINUE;
		}
		// Repoint the A0
		A0 += S1;
		S0 -= S1;
		if (S0 < (1ULL << shift))
		{
			goto CONTINUE;
		}
		*physicalAddress = A0;
		QWORD allocateCount = S0 >> shift;
		if (allocateCount < *pageCount)
		{
			*pageCount = allocateCount;
		}
		allocateCount = *pageCount;
		A0 += allocateCount << shift;
		S0 -= allocateCount << shift;
		beg[0] = A0;
		beg[1] = S0;
		if (S1)
		{
			end[0] = A1;
			end[1] = S1;
			end[2] = 0;
			*((DWORD *) (end + 2)) = 1;
			end += 3;
			mmap[0] = (QWORD) end;
		}
		return;
		CONTINUE:;
		beg += 3;
	}
	// Allocate failed, trap cpu into halt loop
	while (1) __halt();
}
QWORD physical_mapping(QWORD linear)
{
	WORD idx0 = (linear >> 39) & 0x1FF;
	WORD idx1 = (linear >> 30) & 0x1FF;
	WORD idx2 = (linear >> 21) & 0x1FF;
	WORD idx3 = (linear >> 12) & 0x1FF;

	QWORD *L0 = (QWORD *) (__readcr3() | SYSTEM_LNR);
	if (!(L0[idx0] | 1))
	{
		return ~(0ULL);
	}

	QWORD *L1 = (QWORD *) ((L0[idx0] & ~0xFFF) | SYSTEM_LNR);
	if (!(L1[idx1] | 1))
	{
		return ~(0ULL);
	}
	if (L1[idx1] & 0x80)
	{
		return (L1[idx1] & ~0xFFF) + (linear & ((1 << 30) - 1));
	}

	QWORD *L2 = (QWORD *) ((L1[idx1] & ~0xFFF) | SYSTEM_LNR);
	if (!(L2[idx2] | 1))
	{
		return ~(0ULL);
	}
	if (L2[idx2] & 0x80)
	{
		return (L2[idx2] & ~0xFFF) + (linear & ((1 << 21) - 1));
	}

	QWORD *L3 = (QWORD *) ((L2[idx2] & ~0xFFF) | SYSTEM_LNR);
	if (!(L3[idx3] & 1))
	{
		return ~(0ULL);
	}
	return (L3[idx3] & ~0xFFF) + (linear & ((1 << 12) - 1));
}
BIOSAPI BYTE setrsp[] =
{
	0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xFF, 0xFF, // MOV RAX, FFFF800000000000
	0x48, 0x09, 0xC4,                                           // OR RSP, RAX
	0xC3,                                                       // RET
};
void setup_paging(void(*stage)())
{
	QWORD pageCount = 1;
	QWORD *L1 = 0;
	allocate_physical((QWORD *) &L1, 0, &pageCount);
	memset(L1, 0, 0x00001000);

	QWORD *L2 = 0;
	allocate_physical((QWORD *) &L2, 0, &pageCount);
	memset(L2, 0, 0x1000);
	L1[0x100] = (QWORD) L2 | 3;
	L1[0x000] = L1[0x100];
	// 4 * 1G PAGE
	L2[0x000] = (0ULL << 30) | 0x183;
	L2[0x001] = (1ULL << 30) | 0x183;
	L2[0x002] = (2ULL << 30) | 0x183;
	L2[0x003] = (3ULL << 30) | 0x183;

	allocate_physical((QWORD *) &L2, 0, &pageCount);
	memset(L2, 0, 0x1000);
	L1[0x1FF] = (QWORD) L2 | 3;

	QWORD *L3 = 0;
	allocate_physical((QWORD *) &L3, 0, &pageCount);
	memset(L3, 0, 0x1000);
	L2[0x1FF] = (QWORD) L3 | 3;

	DWORD idx2 = 0x1F8;
	QWORD physicalAddress = 0;
	while (idx2 < 0x200)
	{
		pageCount = 0x200 - idx2;
		allocate_physical(&physicalAddress, 1, &pageCount);
		for (QWORD i = 0; i < pageCount; i++)
		{
			L3[idx2++] = physicalAddress | 0x183;
			physicalAddress += 0x00200000;
		}
	}
	idx2 = 0;
	while (idx2 < 8)
	{
		pageCount = 8 - idx2;
		allocate_physical(&physicalAddress, 1, &pageCount);
		for (QWORD i = 0; i < pageCount; i++)
		{
			L3[idx2++] = physicalAddress | 0x183;
			physicalAddress += 0x00200000;
		}
	}
	__writecr3((QWORD) L1);
	((void(*)()) setrsp)();
	((void(*)()) ((QWORD) stage | SYSTEM_LNR))();
}
void setup_screen()
{
	SYSTEM_TABLE.FONT = 0x00001000 | SYSTEM_LNR;
	SYSTEM_TABLE.SCRN = (QWORD) &SCREEN;
	SCREEN.A0 = BOOT_TABLE->A0 | SYSTEM_LNR;
	SCREEN.H = BOOT_TABLE->H;
	SCREEN.V = BOOT_TABLE->V;
	SCREEN.ROW = SCREEN.V;
	SCREEN.CLM = SCREEN.H;
	SCREEN.CSR = 0;
	SCREEN.DM = BOOT_TABLE->DM;
	SCREEN.CM = BOOT_TABLE->CM;
	SCREEN.CLR = 0x0F;
	TEXT_MODE.TEXT = (BYTE *) SCREEN.A0;
	if (SCREEN.DM)
	{
		SCREEN.ROW >>= 4;
		SCREEN.CLM >>= 3;
		TEXT_MODE.TEXT = (BYTE *) (0x00020000 | SYSTEM_LNR);
		memset((void *) SCREEN.A0, 0, SCREEN.H * SCREEN.V * 4);
	}
	memset(TEXT_MODE.TEXT, 0, SCREEN.CLM *SCREEN.ROW * 2);
}
void setup_acpi()
{
	if (!BOOT_TABLE->RSDP)
	{
		OUTPUTTEXT(MSG0002);
		char buf[8] = "RSD PTR ";
		QWORD sig = *((QWORD *) buf);
		QWORD *start = (QWORD *) (0x000E0000 | SYSTEM_LNR);
		while (start < (QWORD *) (0x00100000 | SYSTEM_LNR))
		{
			if (sig == *start)
			{
				BOOT_TABLE->RSDP = (QWORD) start;
			}
			start += 2;
		}
	}
	SYSTEM_TABLE.RSDP = BOOT_TABLE->RSDP | SYSTEM_LNR;
}
BIOSAPI const char HEXDIG[] = "0123456789ABCDEF";
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
void main()
{
	((QWORD *) (__readcr3() | SYSTEM_LNR))[0] = 0;
	SYSTEM_TABLE.BLAT = (0x00010000 | SYSTEM_LNR);
	SYSTEM_TABLE.MMAP = (SYSTEM_TABLE_MMAP | SYSTEM_LNR);
	setup_screen();
	OUTPUTTEXT(MSG0000);

	char buf[4] = { 'A', '0', ' ', 0 };
	OUTPUTTEXT(buf);
	PRINTRAX(SCREEN.A0, 16);
	LINEFEED();

	if (!find_pci())
	{
		// DOS HEADER
		QWORD base = KERNEL_LNR;
		// NT HEADER = ImageBase + [DOS_HEADER + 0x3C]
		QWORD NTHeader = base + *((DWORD *) (base + 0x3C));
		// [NT_HEADER + 0x30] is the absolute liner address of image base
		*((QWORD *) (NTHeader + 0x30)) = base;
		// ImageBase + [NT_HEADER + 0x28] is the address of entry point
		QWORD entry = (QWORD) (base + *((DWORD *) (NTHeader + 0x28)));
		OUTPUTTEXT(MSG0003);
		PRINTRAX(entry, 16);
		LINEFEED();
		BYTE call[24] =
		{
			0x48, 0xBC, 0xD8, 0xFF, 0xFF, 0xFF, // MOV RSP, FFFFFFFFFFFFFFD8
			0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, // JMP QWORD PTR [00000000]
		};
		*((QWORD *) (call + 16)) = entry;
		((void (*)(OS_SYSTEM_TABLE *))call)(&SYSTEM_TABLE);
	}
	else
	{
		OUTPUTTEXT(ERR05);
		while (1) __halt();
	}
}
void mainCRTStartup()
{
	setup_paging(main);
}
void PAINTCURSOR(DWORD cursor)
{
	DWORD i = cursor / SCREEN.CLM;
	DWORD j = cursor % SCREEN.CLM;
	DWORD *pos = (DWORD *) SCREEN.A0;
	pos += SCREEN.H * i * 16;
	pos += j * 8;
	for (DWORD j = 0; j < 16; j++)
	{
		pos[0] = pos[1] = COLOR_PLAETTE[SCREEN.CLR & 0xF];
		pos += SCREEN.H;
	}
}
void PAINTCHAR(BYTE x, BYTE c, DWORD cursor)
{
	DWORD i = cursor / SCREEN.CLM; // pos Y
	DWORD j = cursor % SCREEN.CLM; // pos X
	DWORD *pos = (DWORD *) SCREEN.A0; // 4 byte (32bit) pre pixel
	pos += SCREEN.H * i * 16; // 16 pixel line pre text line
	pos += j * 8; // 8 pixel column pre text column
	BYTE *font = ((BYTE(*)[16]) SYSTEM_TABLE.FONT)[x]; // 8*16 bit map of font
	for (DWORD k = 0; k < 16; k++)
	{
		BYTE bit = font[k];
		for (DWORD p = 0; p < 8; p++)
		{
			if (bit & 0x80)
			{
				// bit is 1, foreground color
				pos[p] = COLOR_PLAETTE[(c >> 0) & 0xF];
			}
			else
			{
				// bit is 0, background color
				pos[p] = COLOR_PLAETTE[(c >> 4) & 0xF];
			}
			bit <<= 1;
		}
		// next pixel line
		pos += SCREEN.H;
	}
}
void SCROLLSCREEN()
{
	memcpy(TEXT_MODE.TEXT, TEXT_MODE.TEXT + (SCREEN.CLM * 2), (SCREEN.CLM * SCREEN.ROW) * 2);
	// If in text mode, text mode cursor is unused.
	// SCREEN.CSR is global current cursor position,
	// TEXT_MODE.CURSOR is only used for software emulation text mode terminal.
	SCREEN.CSR -= SCREEN.CLM;
	if (SCREEN.DM)
	{
		memcpy((void *) SCREEN.A0, (void *) (SCREEN.A0 + (SCREEN.H * 16 * 4)), (SCREEN.H * 16 * (SCREEN.ROW - 1) * 4));
		memset((void *) (SCREEN.A0 + (SCREEN.H * 16 * (SCREEN.ROW - 1) * 4)), 0, SCREEN.H * 16 * 4);
		TEXT_MODE.CURSOR -= SCREEN.CLM;
	}
}
void MOVECURSOR()
{
	if (SCREEN.CSR >= SCREEN.ROW * SCREEN.CLM)
	{
		SCROLLSCREEN();
	}
	if (SCREEN.DM)
	{
		BYTE *ch = ((BYTE(*)[2]) TEXT_MODE.TEXT)[TEXT_MODE.CURSOR];
		PAINTCHAR(ch[0], ch[1], TEXT_MODE.CURSOR);
		PAINTCURSOR(TEXT_MODE.CURSOR = SCREEN.CSR);
	}
	else
	{
		// 0x3D5:0x0E high 8 bit
		__outbyte(0x03D4, 0x0E);
		__outbyte(0x03D5, SCREEN.CSR >> 8);
		// 0x3D5:0x0F low 8 bit
		__outbyte(0x03D4, 0x0F);
		__outbyte(0x03D5, SCREEN.CSR & 0xFF);
	}
}
void OUTPUTTEXT(const char *s)
{
	while (*s)
	{
		char x = *s++;
		switch (x)
		{
			case '\r':
			{
				SCREEN.CSR -= SCREEN.CSR % (SCREEN.CLM);
				MOVECURSOR();
				break;
			}
			case '\n':
			{
				SCREEN.CSR -= SCREEN.CSR % (SCREEN.CLM);
				SCREEN.CSR += SCREEN.CLM;
				MOVECURSOR();
				break;
			}
			default:
			{
				((WORD *) TEXT_MODE.TEXT)[SCREEN.CSR] = (WORD) x | (SCREEN.CLR << 8);
				SCREEN.CSR++;
				MOVECURSOR();
				break;
			}
		}
	}
}
void OUTCHAR(char x)
{
	DWORD y = (BYTE) x;
	OUTPUTTEXT((char *) &y);
}
void CARRIAGERETURN()
{
	OUTCHAR('\r');
}
void LINEFEED()
{
	OUTCHAR('\n');
}
DWORD find_pci()
{
	OUTPUTTEXT(MSG0001);
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
const char *device_name(DWORD id)
{
	DWORD vendor = id & 0xFFFF;
	DWORD device = id >> 16;
	switch (vendor)
	{
		case 0x15AD:
		{
			switch (device)
			{
				case 0x07E0: return DVC07E015AD;
			}
			break;
		}
		case 0x8086:
		{
			switch (device)
			{
				case 0x06D3: return DVC06D38086;
				case 0xA282: return DVCA2828086;
			}
			break;
		}
	}
	return 0;
}
DWORD pci_device(DWORD cmd, DWORD id)
{
	// Read class code
	__outdword(0x0CF8, cmd + 0x08);
	DWORD class = (__indword(0x0CFC) >> 8);

	if (class == 0x010601) // AHCI Controller
	{
		QWORD PCI = 0x28494350;
		BYTE *buf = (BYTE *) &PCI;
		OUTPUTTEXT(buf);
		PRINTRAX(cmd >> 16, 2);
		OUTCHAR('.');
		PRINTRAX((cmd >> 11) & 31, 2);
		OUTCHAR('.');
		PRINTRAX((cmd >> 8) & 7, 1);
		PCI = 0x3A29;
		OUTPUTTEXT(buf);
		const char *dvcName = device_name(id);
		if (dvcName)
		{
			OUTPUTTEXT(dvcName);
		}
		else
		{
			PRINTRAX(id, 8);
		}
		LINEFEED();
		return ahci_controller_setup(cmd);
	}

	return 1;
}
void *TrailingZero(char *beg, char *end)
{
	while (end > beg && *--end == 0x20)
	{
		*end = 0;
	}
	return end + 1;
}
char *LeadingZero(char *beg, char *end)
{
	while (beg < end && *beg == 0x20)
	{
		beg++;
	}
	return beg;
}
DWORD ahci_controller_setup(DWORD dvc)
{
	// Config MMIO
	void* iobase = pci_enable_mmio(dvc, PCI_BASE_ADDRESS_5);
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
	// Use 1 page as buffer
	QWORD page = (0x0000A000 | SYSTEM_LNR);
	while (pi && find)
	{
		if (pi & 1)
		{
			if (check_type(port) == AHCI_DEVICE_SATA)
			{
				// Try to read kernel
				if (!start_cmd(port))
				{
					AHCIIO(port, 0, 1, (void *) physical_mapping(page), ATA_CMD_IDENTIFY_DEVICE);
					BYTE(*buffer)[2] = (BYTE(*)[2]) page;
					for (DWORD i = 0; i < 256; i++)
					{
						BYTE c = buffer[i][0];
						buffer[i][0] = buffer[i][1];
						buffer[i][1] = c;
					}
					PRINTRAX(port - abar->ports, 2);
					OUTCHAR(':');
					OUTCHAR('[');
					BYTE *identify = (BYTE *) (page + 0x14);
					identify[0x14] = 0;
					OUTPUTTEXT(LeadingZero(identify, TrailingZero(identify, identify + 0x14)));
					OUTCHAR(']');
					OUTCHAR('[');
					identify = (BYTE *) (page + 0x36);
					identify[0x28] = 0;
					OUTPUTTEXT(LeadingZero(identify, TrailingZero(identify, identify + 0x28)));
					OUTCHAR(']');
					LINEFEED();
					find &= LoadingSATA(port, page);
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
void* pci_enable_mmio(DWORD device, DWORD addr)
{
	// Read BAR
	__outdword(0x0CF8, device + addr);
	QWORD bar = __indword(0x0CFC);
	// Check BAR PIO Mode
	if (bar & 1)
	{
		OUTPUTTEXT(ERR00);
		return 0;
	}
	bar &= ~0xF;
	bar |= SYSTEM_LNR;
	// Use base as mmio address, write to BAR
	// __outdword(0x0CF8, device + addr);
	// __outdword(0x0CFC, base | (bar & 0xF));
	// Read Command register
	__outdword(0x0CF8, device + PCI_COMMAND);
	WORD cmd = __inword(0x0CFC);
	// Set Memory Space Enable bit
	cmd |= PCI_COMMAND_MEMORY;
	// Write to command register
	__outdword(0x0CF8, device + PCI_COMMAND);
	__outword(0x0CFC, cmd);
	// return (void*)(QWORD)(base);
	return (void *) (QWORD) bar;
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
DWORD start_cmd(HBA_PORT* port)
{
	// Use 2 pages
	//QWORD addr = 0x00008000;
	// Stop command engine
	stop_cmd(port);

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
	OUTPUTTEXT(ERR01);
	return 2;
}
DWORD AHCIIO(HBA_PORT* port, QWORD sector, WORD count, void* buffer, DWORD cmd)
{
	if (!count) return 0;
	if (count > 128) return 1;

	// Clear pending interrupt bits
	port->is = -1;
	int slot = 0;// ahci_find_cmdslot(port);
	if (slot == -1)
		return 2;

	HBA_CMD_HEADER *cmdh = (HBA_CMD_HEADER *) ((port->clb | ((QWORD) port->clbu << 32)) | 0xFFFF800000000000ULL);
	cmdh += slot;
	// Command FIS size
	cmdh->cfl = 5; // sizeof(FIS_REG_H2D) / sizeof(DWORD)
	// Read from device
	cmdh->w = 0;
	// cmdh->c = 1;
	// cmdh->p = 1;
	// PRDT entries count
	cmdh->prdtl = ((count - 1) >> 4) + 1; // UPPER BOUND (count / 4)

	HBA_CMD_TBL *tbl = (HBA_CMD_TBL *) ((cmdh->ctba | ((QWORD) cmdh->ctbau << 32)) | 0xFFFF800000000000ULL);
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
	fis->command = cmd;
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
DWORD LoadingSATA(HBA_PORT* port, QWORD page)
{
	// Read LBA 1: GPT Header
	if (AHCIIO(port, 1, 1, (void *) physical_mapping(page), ATA_CMD_READ_DMA_EX))
	{
		OUTPUTTEXT(ERR02);
		return 1;
	}

	// Assume GPT and check DISK GUID
	QWORD *guid = (QWORD *) (page + 0x38);
	if (guid[0] != BOOT_TABLE->GUID0[0] && guid[1] != BOOT_TABLE->GUID0[1])
	{
		return 1;
	}

	// FIND NTFS SYSTEM PART
	// Read partition table
	if (AHCIIO(port, *((QWORD *) (page + 0x48)), 1, (void *) physical_mapping(page), ATA_CMD_READ_DMA_EX))
	{
		OUTPUTTEXT(ERR02);
		return 1;
	}
	memset((void*)(page + 0x200), 0, 0x200);
	QWORD partEntry = page;
	for (; partEntry < page + 0x200; partEntry += 0x80)
	{
		QWORD* partGUID = (QWORD*)(partEntry + 0x10);
		if (!(partGUID[0] | partGUID[1]))
			break;

		if (partGUID[0] == BOOT_TABLE->GUID1[0] && partGUID[1] == BOOT_TABLE->GUID1[1])
			break;
	}
	if (!(((QWORD*)(partEntry + 0x10))[0] | ((QWORD*)(partEntry + 0x10))[1]))
	{
		OUTPUTTEXT(ERR03);
		return 1;
	}

	// Partition start sector LBA
	QWORD partiionSector = *((QWORD *) (partEntry + 0x20));
	// Read partition first sector
	if (AHCIIO(port, partiionSector, 1, (void *) physical_mapping(page), ATA_CMD_READ_DMA_EX))
	{
		OUTPUTTEXT(ERR02);
		return 1;
	}
	NTFS_BPB BPB;
	memcpy(&BPB, (void *) (page), sizeof(NTFS_BPB));

	// $MFT LBA
	QWORD mftsector = BPB.hidden + (BPB.MFT * BPB.cluster);
	// Read 2 sector file record of KERNEL.DLL file
	if (AHCIIO(port, mftsector + (BOOT_TABLE->MFT << 1), 2, (void *) physical_mapping(page), ATA_CMD_READ_DMA_EX))
	{
		OUTPUTTEXT(ERR02);
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
		OUTPUTTEXT(ERR04);
		return 1;
	}

	// Resolve run list
	attribute += *((WORD*)(attribute + 0x20));
	// Previous run list LBA
	QWORD RUNLIST = BPB.hidden;
	// Physical address of dest, kernel area
	QWORD dst = KERNEL_LNR;
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
			if (AHCIIO(port, sector, BPB.cluster, (void *) physical_mapping(dst), ATA_CMD_READ_DMA_EX))
			{
				OUTPUTTEXT(ERR02);
				return 1;
			}

			sector += BPB.cluster;
			dst += ((QWORD)BPB.cluster) << 9;
		}
	}
	return 0;
}