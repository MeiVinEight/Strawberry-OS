#ifndef __KERNEL_SEGMENT_H__
#define __KERNEL_SEGMENT_H__

#include <types.h>

typedef struct _SEGMENT64
{
	DWORD RSV0;   // L0 and A0 in Protected-Mode
	BYTE  RSV1;   // A1 in Protected-Mode
	BYTE  A:1;    // Accessed, set by CPU
	BYTE  RW:1;   // Write for Data Segment, Read for Code Segment
	BYTE  DC:1;   // Direction(Data)/Conforming(Code)
	BYTE  E:1;    // Executable
	BYTE  S:1;    // If system segment, clear this bit
	BYTE  DPL:2;  // Segment privilege
	BYTE  P:1;    // Present
	BYTE  RSV2:4; // L1 in Protected-Mode
	BYTE  RSV3:1;
	BYTE  L:1;    // Set to 1 if Long-Mode code segment
	BYTE  DB:1;   // 16/32-bit segment
	BYTE  G:1;    // Granularity, 4K if set to 1
	BYTE  RSV4;   // A2
} SEGMENT64;
#pragma pack(push, 2)
typedef struct _GDTR64
{
	WORD L;
	QWORD A;
} GDTR64;
#pragma pack(pop)

extern SEGMENT64 GDT[];
void setup_segment();

#endif