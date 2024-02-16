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
	0x59,             // POP RAX
	0x48, 0x8B, 0xC4, // MOV RAX, RSP
	0x48, 0xFF, 0xE1, // JMP RAX
};
/*
CODEDECL const BYTE __cli[] =
{
	0xFA, // CLI
	0xC3, // RET
};
*/
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
	0x48, 0x8B, 0xF9, // MOV    RDI, RCX
	0x48, 0x63, 0xC2, // MOVSXD RAX, EDX
	0x49, 0x8B, 0xC8, // MOV    RCX, R8
	0x4C, 0x8B, 0xC7, // MOV    R8,  RDI
	0xF3, 0xAA,       // REP STOSB
	0x49, 0x8B, 0xC0, // MOV    RAX, R8
	0x5F,             // POP    RDI
	0xC3,             // RETN
};