#ifndef __KERNEL_INTERRUPT_H__
#define __KERNEL_INTERRUPT_H__

#include <types.h>

#define IRQ_INT 0x20

typedef struct _INTERRUPT64
{
	WORD A0;     // Address [0:15]
	WORD S;      // Segment Selector
	BYTE IST:3;  // Interrupt Stack Table
	BYTE RSV0:5; // 0
	BYTE TYPE:4; // See Table 3-2. System-Segment and Gate-Descriptor Types
	BYTE RSV1:1; // 0
	BYTE DPL:2;  // Privilege
	BYTE P:1;    // Present
	WORD A1;     // Address [16:31]
	DWORD A2;    // Address [32:63]
	DWORD RSV2;  // 0
} INTERRUPT64;
typedef struct _IDTR64
{
	WORD Limit;
	WORD Base[4];
} IDTR64;
typedef struct _INTERRUPT_STACK
{
	// General-purpose registers
	QWORD R15;
	QWORD R14;
	QWORD R13;
	QWORD R12;
	QWORD R11;
	QWORD R10;
	QWORD R9;
	QWORD R8;
	QWORD RDI;
	QWORD RSI;
	QWORD RBP;
	QWORD RBX;
	QWORD RDX;
	QWORD RCX;
	QWORD RAX;
	// Interrupt code
	QWORD INT;
	// Error code
	QWORD ERROR;
	// Interrupt auto push, RIP, CS, EFLAGS
	QWORD RIP;
	QWORD CS;
	QWORD EFLAGS;
} INTERRUPT_STACK;

extern void (*interrupt_eoi)();
extern DWORD USEAPIC;
extern INTERRUPT64 IDT[];

void setup_interrupt();
void register_interrupt(BYTE id, void (*routine)(INTERRUPT_STACK*));

#endif