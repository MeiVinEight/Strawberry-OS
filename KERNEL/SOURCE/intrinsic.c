#include <declspec.h>
#include <types.h>

/*
CODEDECL const BYTE __setrsp[] =
{
	0x58,             // POP RAX
	0x48, 0x8B, 0xE1, // MOV RSP, RCX
	0x48, 0xFF, 0xE0, // JMP RAX
};
*/
CODEDECL const BYTE __getrsp[] =
{
	0x59,             // POP RCX
	0x48, 0x8B, 0xC4, // MOV RAX, RSP
	0x48, 0xFF, 0xE1, // JMP RCX
};
CODEDECL const BYTE __cli[] =
{
	0xFA, // CLI
	0xC3, // RET
};
CODEDECL const BYTE __sti[] =
{
	0xFB, // STI
	0xC3, // RET
};
CODEDECL char memset[] =
/*
void *__cdecl memset(void *, int, unsigned long long)
*/
{
	0x57,             // PUSH   RDI
	0x41, 0x50,       // PUSH   R8
	0x48, 0x8B, 0xF9, // MOV    RDI, RCX
	0x48, 0x63, 0xC2, // MOVSXD RAX, EDX
	0x49, 0x8B, 0xC8, // MOV    RCX, R8
	0x4C, 0x8B, 0xC7, // MOV    R8,  RDI
	0xF3, 0xAA,       // REP STOSB
	0x49, 0x8B, 0xC0, // MOV    RAX, R8
	0x41, 0x58,       // POP    R8
	0x5F,             // POP    RDI
	0xC3,             // RETN
};
CODEDECL char memsetq[] =
/*
void *__cdecl memsetq(void *, unsigned long long, unsigned long long)
*/
{
	0x57,             // PUSH   RDI
	0x41, 0x50,       // PUSH   R8
	0x48, 0x8B, 0xF9, // MOV    RDI, RCX
	0x48, 0x8B, 0xC2, // MOV    RAX, RDX
	0x49, 0x8B, 0xC8, // MOV    RCX, R8
	0x4C, 0x8B, 0xC7, // MOV    R8,  RDI
	0xF3, 0x48, 0xAB, // REP STOSQ
	0x49, 0x8B, 0xC0, // MOV    RAX, R8
	0x41, 0x58,       // POP    R8
	0x5F,             // POP    RDI
	0xC3,             // RETN
};
CODEDECL char memcpy[] =
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
CODEDECL char __lgdt[] =
{
	0x0F, 0x01, 0x11, // LGDT [RCX]
	0xC3              // RET
};
CODEDECL char memcpyq[] =
/*
void *__cdecl memcpyq(void *, const void *, unsigned long long)
*/
{
	0x56,             // PUSH RSI
	0x57,             // PUSH RDI
	0x48, 0x8B, 0xC1, // MOV  RAX, RCX
	0x48, 0x8B, 0xF9, // MOV  RDI, RCX
	0x48, 0x8B, 0xF2, // MOV  RSI, RDX
	0x49, 0x8B, 0xC8, // MOV  RCX, R8
	0xF3, 0x48, 0xA5, // REP MOVSQ
	0x5F,             // POP  RDI
	0x5E,             // POP  RSI
	0xC3,             // RETN
};
WORD __reverse16(WORD x)
{
	return (x << 8) | (x >> 8);
}
DWORD __reverse32(DWORD x)
{
	DWORD y = 0;
	WORD *a = (WORD *) &x;
	WORD *b = (WORD *) &y;
	b[0] = __reverse16(a[1]);
	b[1] = __reverse16(a[0]);
	return y;
}
QWORD __reverse64(QWORD x)
{
	QWORD y = 0;
	DWORD *a = (DWORD *) &x;
	DWORD *b = (DWORD *) &y;
	b[0] = __reverse32(a[1]);
	b[1] = __reverse32(a[0]);
	return y;
}