#include "uefi.h"
#include "intrinsic.h"

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

#define CRT_ADDR_REG 0x3D4
#define CRT_DATA_REG 0x3D5

#define CRT_START_ADDR_H 0xC
#define CRT_START_ADDR_L 0xD
#define CRT_CURSOR_H 0xE
#define CRT_CURSOR_L 0xF

UEFIAPI QWORD EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID[2]     = {0x4A3823DC9042A9DE, 0x6A5180D0DE7AFB96};
UEFIAPI QWORD EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID[2]  = {0x11d26459964e5b22, 0x3b7269c9a000398e};
UEFIAPI QWORD EFI_FILE_INFO_GUID[2]                    = {0x11d26d3f09576e92, 0x3b7269c9a000398E};
UEFIAPI QWORD EFI_ACPI_20_TABLE_GUID[2]                = {0x11D3E4F18868E871, 0x81883CC7800022BC};

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
	BYTE *TEXT;
	DWORD CURSOR;
} TEXT_MODE_MEMORY;
typedef struct _MEMORY_REGION
{
	QWORD A;
	QWORD L;
	DWORD F;
	DWORD X;
} MEMORY_REGION;
TEXT_MODE_MEMORY TEXT_MODE;
CONSOLE_SCREEN SCREEN;
UEFIAPI DWORD COLOR_PLAETTE[] =
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
UEFIAPI const char MEMORY_TYPE[] =
"EfiReservedMemoryType     \0"
"EfiLoaderCode             \0"
"EfiLoaderData             \0"
"EfiBootServicesCode       \0"
"EfiBootServicesData       \0"
"EfiRuntimeServicesCode    \0"
"EfiRuntimeServicesData    \0"
"EfiConventionalMemory     \0"
"EfiUnusableMemory         \0"
"EfiACPIReclaimMemory      \0"
"EfiACPIMemoryNVS          \0"
"EfiMemoryMappedIO         \0"
"EfiMemoryMappedIOPortSpace\0"
"EfiPalCode                \0"
"EfiPersistentMemory       \0"
"EfiUnacceptedMemoryType   \0"
"EfiMaxMemoryType          ";

void LoadFont()
{
	SYSTEM_TABLE->BootServices->LocateProtocol(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID, 0, &SFSP);
	EFI_FILE_PROTOCOL *volume = 0;
	EFI_FILE_PROTOCOL *font = 0;
	SFSP->OpenVolume(SFSP, &volume);
	volume->Open(volume, &font, L"\\FONT.BIN", 1, 0);
	QWORD length = 4096;
	font->Read(font, &length, (void *) 0x00001000);
	font->Close(font);
	volume->Close(volume);
}
void setup_console(EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics)
{
	LoadFont();
	SCREEN.A0 = graphics->Mode->FrameBufferBase;
	SCREEN.H = graphics->Mode->Info->HorizontalResolution;
	SCREEN.V = graphics->Mode->Info->VerticalResolution;
	SCREEN.ROW = SCREEN.V >> 4;
	SCREEN.CLM = SCREEN.H >> 3;
	SCREEN.CSR = 0;
	SCREEN.DM = 1;
	SCREEN.CM = 1;
	SCREEN.CLR = 0x0F;
	TEXT_MODE.TEXT = (BYTE *) 0x00020000;
	memset(TEXT_MODE.TEXT, 0, SCREEN.ROW * SCREEN.CLM * 2);
	memset((void *) SCREEN.A0, 0, SCREEN.H * SCREEN.V * 4);
}
void FLUSHVIDEO()
{
	// memcpy((void *) SCREEN.A0, (void *) SCREEN.A1, SCREEN.H * SCREEN.V * 4);
}
void FLUSHLINE(DWORD start, DWORD count)
{
	/*
	DWORD offset = SCREEN.H * 4 * 16 * start;
	memcpy((void *) (SCREEN.A0 + offset), (void *) (SCREEN.A1 + offset), SCREEN.H * 4 * 16 * count);
	*/
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
	DWORD i = cursor / SCREEN.CLM;
	DWORD j = cursor % SCREEN.CLM;
	DWORD *pos = (DWORD *) SCREEN.A0;
	pos += SCREEN.H * i * 16;
	pos += j * 8;
	BYTE *font = ((BYTE(*)[16]) 0x00001000)[x];
	for (DWORD k = 0; k < 16; k++)
	{
		BYTE bit = font[k];
		for (DWORD p = 0; p < 8; p++)
		{
			if (bit & 0x80)
			{
				pos[p] = COLOR_PLAETTE[(c >> 0) & 0xF];
			}
			else
			{
				pos[p] = COLOR_PLAETTE[(c >> 4) & 0xF];
			}
			bit <<= 1;
		}
		pos += SCREEN.H;
	}
}
void SCROLLSCREEN()
{
	memcpy((void *) SCREEN.A0, (void *) (SCREEN.A0 + (SCREEN.H * 16 * 4)), (SCREEN.H * 16 * (SCREEN.ROW - 1) * 4));
	memset((void *) (SCREEN.A0 + (SCREEN.H * 16 * (SCREEN.ROW - 1) * 4)), 0, SCREEN.H * 16 * 4);
	memcpy((void *) (TEXT_MODE.TEXT), TEXT_MODE.TEXT + (SCREEN.CLM * 2), (SCREEN.CLM * SCREEN.ROW) * 2);
	SCREEN.CSR -= SCREEN.CLM;
	TEXT_MODE.CURSOR -= SCREEN.CLM;
	FLUSHVIDEO();
}
void MOVECURSOR()
{
	if (SCREEN.CSR >= SCREEN.ROW * SCREEN.CLM)
	{
		SCROLLSCREEN();
	}
	BYTE *ch = ((BYTE(*)[2]) TEXT_MODE.TEXT)[TEXT_MODE.CURSOR];
	PAINTCHAR(ch[0], ch[1], TEXT_MODE.CURSOR);
	PAINTCURSOR(TEXT_MODE.CURSOR = SCREEN.CSR);
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
				FLUSHLINE((SCREEN.CSR / SCREEN.CLM) - 1, 2);
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
void PRINTMEMORY(void *b, QWORD length)
{
	BYTE *x = (BYTE *) b;
	QWORD align = (((QWORD) b) & 0xF);
	x -= align;
	length += align;
	for (QWORD i = 0; i < length;)
	{
		if (!(i & 0xF))
		{
			PRINTRAX(((QWORD) x) + i, 16);
		}
		OUTCHAR(' ');
		if (i < align)
		{
			OUTCHAR(' ');
			OUTCHAR(' ');
		}
		else
		{
			PRINTRAX(x[i], 2);
		}
		i++;
		if (i < length && !(i & 0xF))
		{
			LINEFEED();
		}
	}
}
void OUTPUTWORD(QWORD x)
{
	if (x)
	{
		char buf[33];
		buf[32] = 0;
		DWORD idx = 32;
		while (x)
		{
			buf[--idx] = (x % 10) + '0';
			x /= 10;
		}
		OUTPUTTEXT(buf + idx);
		return;
	}
	OUTCHAR('0');
}
void loop()
{
	while (1) __halt();
}
QWORD jmp(QWORD(*entry)())
{
	__cli();
	char idtr[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	__lidt(idtr);
	// while (1) __halt();

	__setrsp(0x00020000 - 0x28);
	return entry();
}
void OutputMemoryMap(EFI_MEMORY_DESCRIPTOR *memory)
{
	PRINTRAX(memory->Type, 2);
	OUTCHAR(':');
	OUTPUTTEXT(MEMORY_TYPE + (27 * memory->Type));
	OUTCHAR(':');
	PRINTRAX(memory->PhysicalStart, 16);
	OUTPUTTEXT(" - ");
	PRINTRAX(memory->PhysicalStart + memory->NumberOfPages * 4096, 16);
}
void SortMemoryMap(QWORD map, DWORD count, QWORD descSize)
{
	void *desc = 0;
	SYSTEM_TABLE->BootServices->AllocatePool(EfiBootServicesData, descSize, &desc);
	for (DWORD i = 0; i < count; i++)
	{
		DWORD min = i;
		EFI_MEMORY_DESCRIPTOR *minEntry = (EFI_MEMORY_DESCRIPTOR *) (map + min * descSize);
		for (DWORD j = i + 1; j < count; j++)
		{
			EFI_MEMORY_DESCRIPTOR *entry = (EFI_MEMORY_DESCRIPTOR *) (map + j * descSize);
			if (entry->PhysicalStart < minEntry->PhysicalStart)
			{
				min = j;
				minEntry = entry;
			}
		}
		if (min != i)
		{
			memcpy(desc, (void *) (map + i * descSize), descSize);
			memcpy((void *) (map + i * descSize), minEntry, descSize);
			memcpy(minEntry, desc, descSize);
		}
	}
	SYSTEM_TABLE->BootServices->FreePool(desc);
}
DWORD CompactMemoryMap(QWORD map, DWORD count, QWORD descSize)
{
	DWORD i = 0;
	EFI_MEMORY_DESCRIPTOR *ei = (EFI_MEMORY_DESCRIPTOR *) (map + i * descSize);
	for (DWORD j = 1; j < count; j++)
	{
		EFI_MEMORY_DESCRIPTOR *ej = (EFI_MEMORY_DESCRIPTOR *) (map + j * descSize);
		if (ej->Type == ei->Type && (ei->PhysicalStart + ei->NumberOfPages * 4096 == ej->PhysicalStart))
		{
			ei->NumberOfPages += ej->NumberOfPages;
		}
		else
		{
			i++;
			ei = (EFI_MEMORY_DESCRIPTOR *) (map + i * descSize);
			if (i != j)
			{
				memcpy(ei, ej, descSize);
			}
		}
	}
	return i + 1;
}

typedef struct _GraphicsMode
{
	WORD H;
	WORD V;
} GraphicsMode;
GraphicsMode MODE[255];
void OutputText(const char *s)
{
	WORD buf[2] = {0, 0};
	while (*s)
	{
		buf[0] = (unsigned char) *s++;
		OutputString(buf);
	}
}
void OutputNumber(QWORD x)
{
	char buf[20] = {'0', 0};
	char *num = buf;
	if (x)
	{
		buf[19] = 0;
		int idx = 19;
		while (idx && x)
		{
			buf[--idx] = '0' + (x % 10);
			x /= 10;
		}
		num += idx;
	}
	OutputText(num);
}
int DetectingScreen(EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics)
{
	int defaultMode = graphics->Mode->Mode;
	QWORD sqr = graphics->Mode->Info->HorizontalResolution * graphics->Mode->Info->VerticalResolution;
	int usedMode = defaultMode;
	char sep[2] = { 0, 0 };
	for (int i = 0; i < graphics->Mode->MaxMode; i++)
	{
		graphics->SetMode(graphics, i);
		MODE[i].H = graphics->Mode->Info->HorizontalResolution;
		MODE[i].V = graphics->Mode->Info->VerticalResolution;
		QWORD x = MODE[i].H * MODE[i].V;
		if (x > sqr)
		{
			sqr = x;
			usedMode = i;
		}
	}
	graphics->SetMode(graphics, defaultMode);
	for (int i = 0; i < graphics->Mode->MaxMode; i++)
	{
		OutputNumber(i);
		sep[0] = ':';
		OutputText(sep);
		OutputNumber(MODE[i].H);
		sep[0] = 'x';
		OutputText(sep);
		OutputNumber(MODE[i].V);
		sep[0] = '\r';
		OutputText(sep);
		sep[0] = '\n';
		OutputText(sep);
	}
	return usedMode;
}

QWORD EFIMainCRTStartup(void *handle, EFI_SYSTEM_TABLE *systemTable)
{
	SYSTEM_TABLE = systemTable;
	SYSTEM_TABLE->BootServices->SetWatchdogTimer(0, 0, 0, 0);

	EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics = 0;
	SYSTEM_TABLE->BootServices->LocateProtocol(EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID, 0, &graphics);

	graphics->SetMode(graphics, DetectingScreen(graphics));
	OutputText("Strawberry-EFI\r\n");
	// memset((void *) graphics->Mode->FrameBufferBase, 0, graphics->Mode->FrameBufferSize);
	QWORD screenSize = graphics->Mode->Info->HorizontalResolution * graphics->Mode->Info->VerticalResolution;
	for (DWORD i = 0; i < screenSize; i++)
	{
		((DWORD *) graphics->Mode->FrameBufferBase)[i] = 0;
	}

	setup_console(graphics);
	SCREEN.CLR = 0x0B;
	OUTPUTTEXT("Strawberry-EFI ");
	SCREEN.CLR = 0x0F;
	OUTPUTWORD(SCREEN.H);
	OUTCHAR('x');
	OUTPUTWORD(SCREEN.V);
	OUTCHAR(':');
	PRINTRAX(graphics->Mode->Info->PixelFormat, 2);
	LINEFEED();

	OUTPUTTEXT("GOP:");
	PRINTRAX(graphics->Mode->FrameBufferBase, 16);
	OUTCHAR('[');
	PRINTRAX(graphics->Mode->FrameBufferSize, 16);
	OUTCHAR(']');
	LINEFEED();

	EFI_FILE_PROTOCOL *volume = 0;
	EFI_FILE_PROTOCOL *osboot = 0;
	SFSP->OpenVolume(SFSP, &volume);
	volume->Open(volume, &osboot, L"\\STBRBOOT", 1, 0);
	BYTE info[128];
	memset(info, 0, 128);
	QWORD length = 128;
	osboot->GetInfo(osboot, EFI_FILE_INFO_GUID, &length, info);
	BYTE *dst = (BYTE *) 0x00010000;
	while (((QWORD *) info)[1])
	{
		length = ((QWORD *) info)[1];
		osboot->Read(osboot, &length, dst);
		((QWORD *) info)[1] -= length;
		dst += length;
	}
	osboot->Close(osboot);
	volume->Close(volume);
	
	STBRBOOT *boot = (STBRBOOT *) 0x00010000;
	boot->DM = SCREEN.DM;
	boot->CM = SCREEN.CM;
	boot->H = SCREEN.H;
	boot->V = SCREEN.V;
	boot->A0 = SCREEN.A0;

	for (QWORD i = 0; i < SYSTEM_TABLE->NumberOfTableEntries; i++)
	{
		if (
			SYSTEM_TABLE->ConfigurationTable[i].GUID[0] == EFI_ACPI_20_TABLE_GUID[0] &&
			SYSTEM_TABLE->ConfigurationTable[i].GUID[1] == EFI_ACPI_20_TABLE_GUID[1]
			)
		{
			OUTPUTTEXT("ACPI 2.0 RSD PTR ");
			PRINTRAX((QWORD) SYSTEM_TABLE->ConfigurationTable[i].TABLE, 16);
			LINEFEED();
			boot->RSDP = (QWORD) SYSTEM_TABLE->ConfigurationTable[i].TABLE;
			break;
		}
	}

	QWORD image = 0x00010000 + boot->LONGMODE;
	QWORD NTHeader = image + *((DWORD *) (image + 0x3C));
	*((QWORD *) (NTHeader + 0x30)) = image;
	QWORD entry = image + *((DWORD *) (NTHeader + 0x28));
	PRINTRAX(entry, 16);
	LINEFEED();

	QWORD memMapSize = 0;
	EFI_MEMORY_DESCRIPTOR *MemoryMap = 0;
	QWORD MapKey = 0;
	QWORD MapDescSize = 0;
	DWORD MapVersion = 0;
	SYSTEM_TABLE->BootServices->GetMemoryMap(&memMapSize, MemoryMap, &MapKey, &MapDescSize, &MapVersion);
	
	QWORD totalSize = memMapSize + sizeof(EFI_MEMORY_DESCRIPTOR) * 20;
	SYSTEM_TABLE->BootServices->AllocatePool(EfiRuntimeServicesData, totalSize, &MemoryMap);
	memset(MemoryMap, 0, totalSize);
	QWORD ret = SYSTEM_TABLE->BootServices->GetMemoryMap(&totalSize, MemoryMap, &MapKey, &MapDescSize, &MapVersion);
	if (ret)
	{
		SCREEN.CLR = 0x0C;
		OUTPUTTEXT("CANNOT GET MEMORY MAP\n");
		SYSTEM_TABLE->BootServices->FreePool(MemoryMap);
		loop();
	}
	QWORD entries = totalSize / MapDescSize;
	PRINTRAX(entries, 4);
	OUTPUTTEXT(" Memory Map\n");
	QWORD MMA = (QWORD) MemoryMap;
	SortMemoryMap(MMA, entries, MapDescSize);
	entries = CompactMemoryMap(MMA, entries, MapDescSize);
	MEMORY_REGION **end = (MEMORY_REGION **) 0x3018;
	MEMORY_REGION *beg = (MEMORY_REGION *) 0x3020;
	memset(beg, 0, sizeof(MEMORY_REGION));
	beg->F = 0xFF;
	// OUTPUTTEXT("Base Address       Length             Type\n");
	for (QWORD i = 0; i < entries; i++)
	{
		MEMORY_REGION region;
		EFI_MEMORY_DESCRIPTOR *memory = (EFI_MEMORY_DESCRIPTOR *) MMA;
		/*
		PRINTRAX(memory->PhysicalStart, 16);
		OUTPUTTEXT(" | ");
		PRINTRAX(memory->NumberOfPages << 12, 16);
		OUTPUTTEXT(" | ");
		OUTPUTTEXT(MEMORY_TYPE + 27 * memory->Type);
		LINEFEED();
		*/
		//OutputMemoryMap(memory);
		//LINEFEED();
		MMA += MapDescSize;
		region.A = memory->PhysicalStart;
		region.L = memory->NumberOfPages * 4096;
		region.X = 0;
		switch (memory->Type)
		{
			case EfiReservedMemoryType:
			case EfiMemoryMappedIO:
			case EfiMemoryMappedIOPortSpace:
				region.F = 2;// BIOS Reserved
				break;
			case EfiACPIReclaimMemory:
				region.F = 3;// BIOS ACPI Reclaimable Memory
				break;
			case EfiACPIMemoryNVS:
				region.F = 4;// BIOS ACPI NVS Memory
				break;
			case EfiUnusableMemory:
				region.F = 5;// BIOS BAD Memory
				break;
			default:
				region.F = 1;// Free
				break;
		}
		if (beg->F == 0xFF || (beg->F == region.F && beg->A + beg->L == region.A))
		{
			if (beg->F == 0xFF)
			{
				beg->F = region.F;
			}
			beg->L += region.L;
		}
		else
		{
			beg++;
			memcpy(beg, &region, sizeof(MEMORY_REGION));
		}
	}
	beg++;
	*end = beg;
	beg = (MEMORY_REGION *) 0x3020;
	OUTPUTTEXT("Base Address       Length             Type\n");
	//          0000000000000000 | 0000000000000000 | 00000000
	while (beg < *end)
	{
		PRINTRAX(beg->A, 16);
		OUTPUTTEXT(" | ");
		PRINTRAX(beg->L, 16);
		OUTPUTTEXT(" | ");
		PRINTRAX(beg->F, 8);
		LINEFEED();
		beg++;
	}
	SYSTEM_TABLE->BootServices->ExitBootServices(handle, MapKey);
	// while (1) __halt();
	return jmp((QWORD(*)()) entry);
}