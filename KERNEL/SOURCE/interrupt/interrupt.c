#include <declspec.h>
#include <interrupt/interrupt.h>
#include <console/console.h>
#include <intrinsic.h>
#include <types.h>
#include <interrupt/8259A.h>
#include <interrupt/apic.h>
#include <memory/heap.h>

#define INTERRUPT_COUNT 256

CODEDECL const char MSG0100[] = "CONSTRUCT IDT";
CODEDECL const char MSG0101[] = "SET IDTR ";

CODEDECL void (*interrupt_eoi)(BYTE);
CODEDECL DWORD USEAPIC = 0;
CODEDECL INTERRUPT64 *IDT;
CODEDECL BYTE(*ISR)[9];
CODEDECL void (**INTERRUPT_ROUTINE)(INTERRUPT_STACK*);
CODEDECL const BYTE __isr[] =
{
	0x50,                         // PUSH RAX
	0x51,                         // PUSH RCX
	0x52,                         // PUSH RDX
	0x53,                         // PUSH RBX
	0x55,                         // PUSH RBP
	0x56,                         // PUSH RSI
	0x57,                         // PUSH RDI
	0x41, 0x50,                   // PUSH R8
	0x41, 0x51,                   // PUSH R9
	0x41, 0x52,                   // PUSH R10
	0x41, 0x53,                   // PUSH R11
	0x41, 0x54,                   // PUSH R12
	0x41, 0x55,                   // PUSH R13
	0x41, 0x56,                   // PUSH R14
	0x41, 0x57,                   // PUSH R15
	0x48, 0x8B, 0xCC,             // MOV RCX, RSP
	0xE8, 0x00, 0x00, 0x00, 0x00, // CALL 00000000
	0x41, 0x5F,                   // POP R15
	0x41, 0x5E,                   // POP R14
	0x41, 0x5D,                   // POP R13
	0x41, 0x5C,                   // POP R12
	0x41, 0x5B,                   // POP R11
	0x41, 0x5A,                   // POP R10
	0x41, 0x59,                   // POP R9
	0x41, 0x58,                   // POP R8
	0x5F,                         // POP RDI
	0x5E,                         // POP RSI
	0x5D,                         // POP RBP
	0x5B,                         // POP RBX
	0x5A,                         // POP RDX
	0x59,                         // POP RCX
	0x58,                         // POP RAX
	0x48, 0x83, 0xC4, 0x10,       // ADD RSP, 10
	0x48, 0xCF,                   // IRETQ
};
void __isr_common(INTERRUPT_STACK *stack)
{
	BYTE id = stack->INT;
	if (interrupt_eoi)
	{
		interrupt_eoi(id);
	}
	// INT
	if (INTERRUPT_ROUTINE[id])
	{
		INTERRUPT_ROUTINE[id](stack);
	}
}
void register_interrupt(BYTE id, void (*routine)(INTERRUPT_STACK*))
{
	// Set routine to interrupt routine list
	INTERRUPT_ROUTINE[id] = routine;

	DWORD erc = 0x00027D00; // ERROR CODE Mask
	BYTE* isrx = ISR[id];
	if ((id < 32) && (erc & (1 << id)))
	{
		// Have error code
		// NOP
		isrx[0x0] = 0x90;
		// NOP
		isrx[0x1] = 0x90;
	}
	else
	{
		// No error code
		// PUSH 00
		isrx[0x0] = 0x6A;
		isrx[0x1] = 0x00;
	}
	// PUSH id
	isrx[0x2] = 0x6A;
	isrx[0x3] = id;
	// JMP __isr
	isrx[0x4] = 0xE9;
	// RVA of __isr
	QWORD rva = (QWORD)(isrx + 0x5);
	*((DWORD*)rva) = (DWORD)(((QWORD)__isr) - (rva + 4));

	// Set isrx to IDT
	QWORD isr0a = (QWORD)isrx;
	IDT[id].A0 = (isr0a >> 0) & 0xFFFF;
	IDT[id].A1 = (isr0a >> 16) & 0xFFFF;
	IDT[id].A2 = (isr0a >> 32);
	IDT[id].P = 1;
}
void __FAULT(INTERRUPT_STACK* stack)
{
	char buf[5] = { '#', 'G', 'P', '\n', 0 };
	if (stack->INT == 0x08)
	{
		buf[1] = 'D';
		buf[2] = 'F';
	}
	BYTE color0 = SCREEN.CLR;
	SCREEN.CLR = 0x0C;
	OUTPUTTEXT(buf);
	SCREEN.CLR = color0;
	buf[0] = 'E';
	buf[1] = 'R';
	buf[2] = 'R';
	buf[3] = '=';
	OUTPUTTEXT(buf);
	PRINTRAX(stack->ERROR, 16);
	LINEFEED();
	buf[0] = 'R';
	buf[1] = 'I';
	buf[2] = 'P';
	OUTPUTTEXT(buf);
	PRINTRAX(stack->RIP, 16);
	LINEFEED();
	while (1) __halt();
}
void setup_interrupt()
{
	OUTPUTTEXT(MSG0100);
	LINEFEED();
	// Init IDT: TYPE=0x0E:Interrupt Gate, S=0x08:Kernel code segment
	IDT = (INTERRUPT64 *) HeapAlloc(HEAPK, sizeof(INTERRUPT64) * INTERRUPT_COUNT);
	memset(IDT, 0, sizeof(INTERRUPT64) * INTERRUPT_COUNT);
	ISR = (BYTE(*)[9]) HeapAlloc(HEAPK, sizeof(ISR[0]) * INTERRUPT_COUNT);
	memset(ISR, 0, sizeof(ISR[0]) * INTERRUPT_COUNT);
	INTERRUPT_ROUTINE = (void (**)(INTERRUPT_STACK *)) HeapAlloc(HEAPK, sizeof(void *) * INTERRUPT_COUNT);
	memset(INTERRUPT_ROUTINE, 0, sizeof(void *) * INTERRUPT_COUNT);
	for (DWORD i = 0; i < 256; i++)
	{
		IDT[i].S = 0x08;
		IDT[i].TYPE = 0x0E;
		register_interrupt(i, 0);
	}

	// Setup interrupt controller
	if (check_apic())
	{
		setup_apic();
		USEAPIC = 1;
	}
	else
	{
		setup_8259A();
	}

	// Calculate RVA from __isr+0x1B to __isr_common
	QWORD rva = (QWORD)(__isr + 0x1B);
	*((DWORD*)rva) = (DWORD)(((QWORD)__isr_common) - (rva + 4));

	// Interrupt exceptions
	register_interrupt(0x08, __FAULT);
	register_interrupt(0x0D, __FAULT);

	// lidt
	IDTR64 idtr;
	idtr.Limit = (sizeof(INTERRUPT64) * INTERRUPT_COUNT) - 1;
	*((QWORD*)&idtr.Base) = ((QWORD)IDT);
	__lidt(&idtr);

	// Output idtr
	idtr.Limit = 0;
	*((QWORD*)&idtr.Base) = 0;
	__sidt(&idtr);
	char separate[2] = { ':', 0 };
	OUTPUTTEXT(MSG0101);
	PRINTRAX(idtr.Limit, 4);
	OUTPUTTEXT(separate);
	PRINTRAX(*((QWORD*)&idtr.Base), 16);
	LINEFEED();

	// Enable interrupt
	__sti();
}