#include <memory/segment.h>
#include <declspec.h>
#include <intrinsic.h>

CODEDECL SEGMENT64 GDT[] =
{
	{.RW = 0, .E = 0, .S = 0, .DPL = 0, .P = 0, .L = 0}, // 0x0000000000000000 RSV
	{.RW = 1, .E = 1, .S = 1, .DPL = 0, .P = 1, .L = 1}, // 0x00209A0000000000 CODE
	{.RW = 1, .E = 0, .S = 1, .DPL = 0, .P = 1, .L = 0}, // 0x0000920000000000 DATA
};
CODEDECL WORD GLOBAL_DESCRIPTOR_COUNT = 0;
CODEDECL BYTE LXS0[] =
{
	0x6A, 0x08,                                                 // PUSH 08
	0x51,                                                       // PUSH RCX
	0x48, 0xCB,                                                 // RETFQ
};
CODEDECL BYTE LXS1[] =
{
	0x31, 0xC9,                                                 // XOR ECX, ECX
	0x66, 0x8E, 0xE1,                                           // MOV FS, CX
	0x66, 0x8E, 0xE9,                                           // MOV GS, CX
	0x83, 0xC1, 0x10,                                           // ADD ECX, 10
	0x66, 0x8E, 0xC1,                                           // MOV ES, CX
	0x66, 0x8E, 0xD1,                                           // MOV SS, CX
	0x66, 0x8E, 0xD9,                                           // MOV DS, CX
	0xC3,                                                       // RET
};
void setup_segment()
{
	GLOBAL_DESCRIPTOR_COUNT = sizeof(GDT) >> 3;
	GDTR64 gdtr;
	gdtr.L = sizeof(GDT) - 1;
	gdtr.A = (QWORD) GDT;
	__lgdt(&gdtr);
	((void(*)(void *)) LXS0)(LXS1);
}