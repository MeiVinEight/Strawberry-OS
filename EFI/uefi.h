#ifndef __UEFI_H__
#define __UEFI_H__


#define EFI_OPEN_PROTOCOL_GET_PROTOCOL          0x00000002


typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned long long QWORD;

typedef enum _EFI_ALLOCATE_TYPE
{
	AllocateAnyPages,
	AllocateMaxAddress,
	AllocateAddress,
	MaxAllocateTyp
} EFI_ALLOCATE_TYPE;
typedef enum _EFI_MEMORY_TYPE
{
	EfiReservedMemoryType,
	EfiLoaderCode,
	EfiLoaderData,
	EfiBootServicesCode,
	EfiBootServicesData,
	EfiRuntimeServicesCode,
	EfiRuntimeServicesData,
	EfiConventionalMemory,
	EfiUnusableMemory,
	EfiACPIReclaimMemory,
	EfiACPIMemoryNVS,
	EfiMemoryMappedIO,
	EfiMemoryMappedIOPortSpace,
	EfiPalCode,
	EfiPersistentMemory,
	EfiUnacceptedMemoryType,
	EfiMaxMemoryType
} EFI_MEMORY_TYPE;
typedef enum _EFI_LOCATE_SEARCH_TYPE
{
	AllHandles,
	ByRegisterNotify,
	ByProtocol
} EFI_LOCATE_SEARCH_TYPE;
typedef enum _EFI_GRAPHICS_PIXEL_FORMAT
{
	PixelRedGreenBlueReserved8BitPerColor,
	PixelBlueGreenRedReserved8BitPerColor,
	PixelBitMask,
	PixelBltOnly,
	PixelFormatMax
} EFI_GRAPHICS_PIXEL_FORMAT;
typedef struct _EFI_TABLE_HEADER
{
	QWORD Signature;
	DWORD Revision;
	DWORD HeaderSize;
	DWORD CRC32;
	BYTE Reserved;
} EFI_TABLE_HEADER;
typedef struct _SIMPLE_TEXT_OUTPUT_MODE
{
	DWORD MaxMode;
	DWORD Mode;
	DWORD Attribute;
	DWORD CursorColumn;
	DWORD CursorRow;
	DWORD CursorVisible;
} SIMPLE_TEXT_OUTPUT_MODE;
typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
{
	QWORD Reset;
	QWORD(*OutputString)(struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *, WORD *);
	QWORD TestString;
	QWORD QueryMode;
	QWORD(*SetMode)(struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *, QWORD);
	QWORD SetAttribute;
	QWORD(*ClearScreen)(struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *);
	QWORD(*SetCursorPosition)(struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *, QWORD, QWORD);
	QWORD EnableCursor;
	SIMPLE_TEXT_OUTPUT_MODE *Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
typedef struct _EFI_MEMORY_DESCRIPTOR
{
	DWORD Type;
	QWORD PhysicalStart;
	QWORD VirtualStart;
	QWORD NumberOfPages;
	QWORD Attribute;
} EFI_MEMORY_DESCRIPTOR;
typedef struct _EFI_BOOT_SERVICES
{
	EFI_TABLE_HEADER Hdr;

	// Task Priority Services
	void *RaiseTPL;
	void *RestoreTPL;

	// Memory Services
	QWORD(*AllocatePages)(EFI_ALLOCATE_TYPE, EFI_MEMORY_TYPE, DWORD, QWORD *);
	QWORD(*FreePages)(QWORD, DWORD);
	QWORD(*GetMemoryMap)(QWORD *, EFI_MEMORY_DESCRIPTOR *, QWORD *, QWORD *, DWORD *);
	QWORD(*AllocatePool)(EFI_MEMORY_TYPE, QWORD, void **);
	QWORD(*FreePool)(void *);

	// Event & Timer Services
	QWORD *CreateEvent;
	QWORD *SetTimer;
	QWORD *WaitForEvent;
	QWORD *SignalEvent;
	QWORD *CloseEvent;
	QWORD *CheckEvent;

	// Protocol Handler Services
	QWORD *InstallProtocolInterface;
	QWORD *ReinstallProtocolInterface;
	QWORD *UninstallProtocolInterface;
	QWORD *HandleProtocol;
	QWORD *Reserved;
	QWORD *RegisterProtocolNotify;
	QWORD(*LocateHandle)(EFI_LOCATE_SEARCH_TYPE, void *, void *, QWORD *, void **);
	QWORD *LocateDevicePath;
	QWORD *InstallConfigurationTable;

	// Image Services
	QWORD *LoadImage;
	QWORD *StartImage;
	QWORD *Exit;
	QWORD *UnloadImage;
	QWORD(*ExitBootServices)(void *, QWORD);

	// Miscellaneous Services
	QWORD *GetNextMonotonicCount;
	QWORD *Stall;
	QWORD(*SetWatchdogTimer)(QWORD, QWORD, QWORD, char *);

	// DriverSupport Services
	QWORD *ConnectController;
	QWORD *DisconnectController;

	// Open and Close Protocol Services
	QWORD(*OpenProtocol)(void *, void *, void **, void *, void *, DWORD);
	QWORD *CloseProtocol;
	QWORD *OpenProtocolInformation;

	// Library Services
	QWORD(*ProtocolPerHandle);
	QWORD(*LocateHandleBuffer);
	QWORD(*LocateProtocol)(void *, void *, void **);

	// Library Services
} EFI_BOOT_SERVICES;
typedef struct _EFI_CONFIGURATION_TABLE
{
	QWORD GUID[2];
	void *TABLE;
} EFI_CONFIGURATION_TABLE;
typedef struct _EFI_SYSTEM_TABLE
{
	EFI_TABLE_HEADER Hdr;
	WORD *FirmwareVendor;
	DWORD FirmwareRevision;
	void *ConsoleInHandle;
	void *ConIn;
	void *ConsoleOutHandle;
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
	void *StandardErrorHandle;
	void *StdErr;
	void *RuntimeServices;
	EFI_BOOT_SERVICES *BootServices;
	QWORD NumberOfTableEntries;
	EFI_CONFIGURATION_TABLE *ConfigurationTable;
} EFI_SYSTEM_TABLE;
typedef struct _EFI_DEVICE_PATH_PROTOCOL
{
	BYTE Type;
	BYTE Subtype;
	WORD Length;
	BYTE Data[0];
} EFI_DEVICE_PATH_PROTOCOL;
typedef struct _EFI_LOADED_IMAGE_PROTOCOL
{
	DWORD Revision;
	void *ParentHandle;
	EFI_SYSTEM_TABLE *SystemTable;

	// Source location of the image
	void *DeviceHandle;
	EFI_DEVICE_PATH_PROTOCOL *FilePath;
	void *Reserved;

	// Image's load options
	DWORD LoadOptionsSize;
	void *LoadOptions;

	// Location where image was loaded
	void *ImageBase;
	QWORD ImageSize;
	EFI_MEMORY_TYPE ImageCodeType;
	EFI_MEMORY_TYPE ImageDataType;
	QWORD(*Unload);
} EFI_LOADED_IMAGE_PROTOCOL;
typedef struct _EFI_DEVICE_PATH_TO_TEXT_PROTOCOL
{
	QWORD(*ConvertDeviceNodeToText);
	QWORD(*ConvertDevicePathToText)(const EFI_DEVICE_PATH_PROTOCOL *, BYTE, BYTE);
} EFI_DEVICE_PATH_TO_TEXT_PROTOCOL;
typedef struct _EFI_BLOCK_IO_MEDIA
{
	DWORD MediaId;
} EFI_BLOCK_IO_MEDIA;
typedef struct _EFI_BLOCK_IO_PROTOCOL
{
	DWORD Revision;
	EFI_BLOCK_IO_MEDIA *Meida;
	QWORD(*Reset);
	QWORD(*ReadBlocks)(struct _EFI_BLOCK_IO_PROTOCOL *, DWORD, QWORD, QWORD, void *);
} EFI_BLOCK_IO_PROTOCOL;
typedef struct _EFI_PIXEL_BITMASK
{
	DWORD MASK[4];
} EFI_PIXEL_BITMASK;
typedef struct _EFI_GRAPHICS_OUTPUT_MODE_INFORMATION
{
	DWORD Version;
	DWORD HorizontalResolution;
	DWORD VerticalResolution;
	EFI_GRAPHICS_PIXEL_FORMAT PixelFormat;
	EFI_PIXEL_BITMASK PixelInformation;
	DWORD PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;
typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE
{
	DWORD MaxMode;
	DWORD Mode;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
	QWORD SizeOfInfo;
	QWORD FrameBufferBase;
	QWORD FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;
typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL
{
	QWORD(*QueryMode)();
	QWORD(*SetMode)(struct _EFI_GRAPHICS_OUTPUT_PROTOCOL *, DWORD);
	QWORD(*Blt)();
	EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;
typedef struct _EFI_FILE_PROTOCOL
{
	QWORD Revision;
	QWORD(*Open)(struct _EFI_FILE_PROTOCOL *, struct _EFI_FILE_PROTOCOL **, WORD *, QWORD, QWORD);
	QWORD(*Close)(struct _EFI_FILE_PROTOCOL *);
	QWORD(*Delete)();
	QWORD(*Read)(struct _EFI_FILE_PROTOCOL *, QWORD *, void *);
	QWORD(*Write)();
	QWORD(*GetPosition)();
	QWORD(*SetPosition)();
	QWORD(*GetInfo)(struct _EFI_FILE_PROTOCOL *, void *, QWORD *, void *);
} EFI_FILE_PROTOCOL;
typedef struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
{
	QWORD Revision;
	QWORD(*OpenVolume)(struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *, EFI_FILE_PROTOCOL **);
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

#pragma section(".text")
#define UEFIAPI __declspec(allocate(".text"))

extern EFI_SYSTEM_TABLE *SYSTEM_TABLE;
extern EFI_DEVICE_PATH_TO_TEXT_PROTOCOL *DPTTP;
extern EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SFSP;

void CARRIAGERETURN();
void LINEFEED();
void OUTCHAR(char);
void OUTPUTTEXT(const char *);
void PRINTRAX(QWORD, BYTE);
void PRINTMEMORY(void *, QWORD);
void OutputString(const WORD *);

#endif