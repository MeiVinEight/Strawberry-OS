#include <declspec.h>

CODEDECL unsigned char SCROLLSCREEN[] =
{
	0x51,                         // PUSH RCX
	0x56,                         // PUSH RSI
	0x57,                         // PUSH RDI
	0xBE, 0xA0, 0x80, 0x0B, 0x00, // MOV ESI, 0x000B80A0
	0xBF, 0x00, 0x80, 0x0B, 0x00, // MOV EDI, 0x000B8000
	0xB9, 0xD0, 0x07, 0x00, 0x00, // MOV ECX, 0x000007D0
	0xFC,                         // CLD
	0xF3, 0x66, 0xA5,             // REP MOVSW
	0x5F,                         // POP RDI
	0x5E,                         // POP RSI
	0x59,                         // POP RCX
	0xC3                          // RET
};