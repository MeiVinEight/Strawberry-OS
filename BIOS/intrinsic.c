#pragma section(".text")

__declspec(allocate(".text")) unsigned char SCROLLPAGE[] =
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
__declspec(allocate(".text")) char memset[] =
/*
void *__cdecl memset(void *, int, unsigned long long)
*/
{
	0x57,             // PUSH   RDI
	0x48, 0x8B, 0xF9, // MOV    RDI, RCX
	0x48, 0x63, 0xC2, // MOVSXD RAX, EDX
	0x49, 0x8B, 0xC8, // MOV    RCX, R8
	0x4C, 0x8B, 0xC7, // MOV    R8,  RDI
	0xF3, 0xAA,       // REP STOSB
	0x49, 0x8B, 0xC0, // MOV    RAX, R8
	0x5F,             // POP    RDI
	0xC3,             // RETN
};
__declspec(allocate(".text")) char memcpy[] =
/*
void *__cdecl memcpy(void *, const void *, unsigned long long)
*/
{
	0x56,             // PUSH RSI
	0x57,             // PUSH RDI
	0x48, 0x8B, 0xC1, // MOV  RAX, RCX
	0x48, 0x8B, 0xF9, // MOV  RDI, RCX
	0x48, 0x8B, 0xF2, // MOV  RSI, RDX
	0x49, 0x8B, 0xC8, // MOV  RCX, R8
	0xF3, 0xA4,       // REP MOVSB
	0x5F,             // POP  RDI
	0x5E,             // POP  RSI
	0xC3,             // RETN
};