#include <interrupt/apic.h>
#include <declspec.h>
#include <types.h>
#include <intrinsic.h>
#include <console/console.h>
#include <interrupt/8259A.h>
#include <interrupt/interrupt.h>
#include <timer/8254.h>
#include <timer/timer.h>
#include <system.h>
#include <memory/page.h>
#include <msr.h>
#include <memory/segment.h>
#include <interrupt/interrupt.h>
#include <memory/page.h>

#define CPUID_FEAT_EDX_APIC (1 << 9)

#define IA32_APIC_BASE_MSR_BSP 0x100 // Processor is a BSP
#define IA32_APIC_BASE_MSR_ENABLE 0x800

#define APIC_SOFTWARE_ENABLE 0x100
#define APIC_SPURIOUS_VECTOR 0xFF

#define APIC_APICID  0x02
#define APIC_APICVER 0x03
#define APIC_TPR     0x08
#define APIC_APR     0x09
#define APIC_PPR     0x0A
#define APIC_EOI     0x0B
#define APIC_RRD     0x0C
#define APIC_LDR     0x0D
#define APIC_DFR     0x0E
#define APIC_SIV     0x0F
#define APIC_ISR     0x10
#define APIC_TMR     0x18
#define APIC_IRR     0x20
#define APIC_ESR     0x28
#define APIC_CMCI    0x2F
#define APIC_ICRL    0x30
#define APIC_ICRH    0x31
#define APIC_LVT0    0x32
#define APIC_LVT1    0x33
#define APIC_LVT2    0x34
#define APIC_LVT3    0x35
#define APIC_LVT4    0x36
#define APIC_LVTE    0x37
#define APIC_TICR    0x38
#define APIC_TCCR    0x39
#define APIC_TDCR    0x3E

#define APIC_ICR_DELIVERY_INIT       0x00000500
#define APIC_ICR_DELIVERY_STARTUP    0x00000600
#define APIC_ICR_DELIVERY_STATUS     0x00001000
#define APIC_ICR_LEVEL_ASSERT        0x00004000
#define APIC_ICR_TRIGGER_MODE_LEVEL  0x00008000
#define APIC_ICR_SHORTHAND_EXCLUDING 0x000C0000

#define APIC_LVT_NMI (1 << 10)
#define APIC_LVT_CLR (1 << 16)

#define APIC_TIMER_IRQ 0
#define APIC_TIMER_MODE_PERIODIC (1 << 17)
#define APIC_TIMER_DCR_1   0xB
#define APIC_TIMER_DCR_2   0x0
#define APIC_TIMER_DCR_4   0x1
#define APIC_TIMER_DCR_8   0x2
#define APIC_TIMER_DCR_16  0x3
#define APIC_TIMER_DCR_32  0x8
#define APIC_TIMER_DCR_64  0x9
#define APIC_TIMER_DCR_128 0xA

CODEDECL const char MSG0400[] = "SETUP APIC\n";
CODEDECL const char MSG0401[] = "SETUP APIC TIMER ";
CODEDECL const char MSG0402[] = "APIC TIMER FREQUENCY ";
CODEDECL const char MSG0403[] = "APIC REGISTER ADDRESS ";
CODEDECL const char MSG0404[] = "ERROR:CANNOT REAL-MODE MEMORY\n";
CODEDECL DWORD (*APIC_REGISTERS)[4];

CODEDECL BYTE AP_BOOT_CODE[] =
{
	0xFA,                                           // CS:0000 CLI
	0xEA, 0x20, 0x00, 0x00, 0x00,                   // CS:0001 JMP FAR CS:0020
	0xEB, 0xFE,                                     // CS:0006 JMP $+00
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // CS:0008 DQ 0000000000000000
	0x00, 0x00, 0x00, 0x00, 0x00, 0x9A, 0x20, 0x00, // CS:0010 DQ 00209A0000000000
	0x00, 0x00, 0x00, 0x00, 0x00, 0x92, 0x00, 0x00, // CS:0018 DQ 0000920000000000
	0x8C, 0xC8,                                     // CS:0020 MOV AX, CS
	0x8E, 0xD8,                                     // CS:0022 MOV DS, AX
	0x8E, 0xC0,                                     // CS:0024 MOV ES, AX
	0x8E, 0xD0,                                     // CS:0026 MOV SS, AX
	0xBC, 0x00, 0x10,                               // CS:0028 MOV SP, 1000
	0x66, 0x33, 0xC0,                               // CS:002B XOR EAX, EAX
	0x8C, 0xC8,                                     // CS:002E MOV AX, CS
	0xB1, 0x04,                                     // CS:0030 MOV CL, 04
	0x66, 0xD3, 0xE0,                               // CS:0032 SHL EAX, CL
	0xBE, 0xA5, 0x00,                               // CS:0035 MOV SI, 00A5
	0x66, 0x01, 0x04,                               // CS:0038 ADD [SI], EAX
	0xE4, 0x92,                                     // CS:0x3B IN AL, 92
	0x0C, 0x02,                                     // CS:003D OR AL, 02
	0xE6, 0x92,                                     // CS:003F OUT 92, AL
	0x66, 0x33, 0xC0,                               // CS:0041 XOR EAX, EAX
	0x66, 0x50,                                     // CS:0044 PUSH EAX
	0x8C, 0xC8,                                     // CS:0046 MOV AX, CS
	0xB1, 0x0C,                                     // CS:0048 MOV CL, 0C
	0xD3, 0xE8,                                     // CS:004A SHR AX, CL
	0x50,                                           // CS:004C PUSH AX
	0x8C, 0xC8,                                     // CS:004D MOV AX, CS
	0xB1, 0x04,                                     // CS:004F MOV CL, 04
	0xD3, 0xE0,                                     // CS:0051 SHL AX, CL
	0xB9, 0x08, 0x00,                               // CS:0053 MOV CX, 0008
	0x03, 0xC1,                                     // CS:0056 ADD AX, CX
	0x50,                                           // CS:0058 PUSH AX
	0xB8, 0x17, 0x00,                               // CS:0059 MOV AX, 0017
	0x50,                                           // CS:005C PUSH AX
	0x8B, 0xF4,                                     // CS:005D MOV SI, SP
	0x3E, 0x0F, 0x01, 0x14,                         // CS:005F LGDT DS:[SI]
	0x83, 0xC4, 0x0A,                               // CS:0063 ADD SP, 000A
	0x66, 0xB8, 0x00, 0x00, 0x10, 0x00,             // CS:0066 MOV EAX, 00100000
	0x66, 0x0F, 0x22, 0x18,                         // CS:006C MOV CR3, EAX
	0x66, 0x0F, 0x20, 0x20,                         // CS:0070 MOV EAX, CR4
	0x66, 0x83, 0xC8, 0x20,                         // CS:0074 OR EAX, 20
	0x66, 0x0D, 0x80, 0x00, 0x00, 0x00,             // CS:0078 OR EAX, 00000080
	0x66, 0x0F, 0x22, 0x20,                         // CS:007E MOV CR4, EAX
	0x66, 0xB9, 0x80, 0x00, 0x00, 0xC0,             // CS:0082 MOV ECX, C0000080
	0x0F, 0x32,                                     // CS:0088 RDMSR
	0x66, 0x0D, 0x00, 0x01, 0x00, 0x00,             // CS:008A OR EAX, 00000100
	0x0F, 0x30,                                     // CS:0090 WRMSR
	0x66, 0x0F, 0x20, 0x00,                         // CS:0092 MOV EAX, CR0
	0x66, 0xB9, 0x01, 0x00, 0x00, 0x80,             // CS:0096 MOV ECX, 80000001
	0x66, 0x0B, 0xC1,                               // CS:009C OR EAX, ECX
	0x66, 0x0F, 0x22, 0x00,                         // CS:009F MOV CR0, EAX
	0x66, 0xEA, 0xAB, 0x00, 0x00, 0x00, 0x08, 0x00, // CS:00A3 JMP FAR 0008:000000AB
	                                                            // [Long-Mode]
	0x31, 0xC9,                                                 // 000000AB XOR ECX, ECX
	0x66, 0x8E, 0xE1,                                           // 000000AD MOV FS, CX
	0x66, 0x8E, 0xE9,                                           // 000000B0 MOV GS, CX
	0x83, 0xC1, 0x10,                                           // 000000B3 ADD ECX, 10
	0x66, 0x8E, 0xC1,                                           // 000000B6 MOV ES, CX
	0x66, 0x8E, 0xD1,                                           // 000000B9 MOV SS, CX
	0x66, 0x8E, 0xD9,                                           // 000000BC MOV DS, CX
	0x48, 0xBC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 000000BF MOV RSP, 0000000000000000
	0xB9, 0x00, 0x00, 0x00, 0x00,                               // 000000C9 MOV ECX, 00000000
	0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,                         // 000000CE JMP QWORD PTR [00000000]
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,             // 000000D4 DQ 0000000000000000
};

void SetAPICAddress()
{
	QWORD apic_base_msr = __readmsr(IA32_APIC_BASE_MSR);
	QWORD apic_base = apic_base_msr & (~0xFFF);
	QWORD apic_base_linear = apic_base | 0xFFFF800000000000ULL;
	// Hardware enable APIC
	// apic_base |= (1 << 11);
	// __writemsr(IA32_APIC_BASE_MSR, apic_base_msr);
	// APIC Registers base
	APIC_REGISTERS = (DWORD(*)[4]) apic_base_linear;
	OUTPUTTEXT(MSG0403);
	PRINTRAX(apic_base_linear, 16);
	LINEFEED();
}
void EOIAPIC(BYTE id)
{
	if (id >= IRQ_INT && id < (IRQ_INT + 0x10))
		APIC_REGISTERS[APIC_EOI][0] = 0;
}

int check_apic()
{
	DWORD cpui[4] = {0};
	__cpuid(cpui, 1);
	return !!(cpui[3] & CPUID_FEAT_EDX_APIC);
}
void SetupAPIC()
{
	disable_8259A();

	OUTPUTTEXT(MSG0400);
	SetAPICAddress();
	// APIC EOI
	INTERRUPT_EOI = EOIAPIC;
	ConfigureAPIC();
}
void setup_apic_timer(DWORD rate)
{
	char text[16];
	text[0] = ' ';
	text[1] = 'H';
	text[2] = 'z';
	text[3] = 0;
	OUTPUTTEXT(MSG0401);
	OUTPUTWORD(rate);
	OUTPUTTEXT(text);
	LINEFEED();
	// Map APIC timer to an interrupt, and by that enable it in one-shot mode
	APIC_REGISTERS[APIC_LVT0][0] = (IRQ_INT + APIC_TIMER_IRQ);
	// Set up divide value to 1
	// See Intel® 64 and IA-32 Architectures Software Developer’s Manual. Volume 2. Figure 11.10
	APIC_REGISTERS[APIC_TDCR][0] = APIC_TIMER_DCR_1;

	// Prepare PIT sleep 50 ms
	__outbyte(PIT2_GATE, (__inbyte(PIT2_GATE) & 0xFD) | 1);
	__outbyte(PIT_CMD, 0B10110010);
	DWORD frq = PIT_FREQUENCY / rate;
	__outbyte(PIT2_DATA, (frq >> 0) & 0xFF);
	__inbyte(0x60);
	__outbyte(PIT2_DATA, (frq >> 8) & 0xFF);
	BYTE al = __inbyte(PIT2_GATE) & 0xFE;
	// Reset PIT one-shot counter (start counting)
	__outbyte(PIT2_GATE, al);
	__outbyte(PIT2_GATE, al | 1);
	// Reset APIC timer (set counter to -1)
	APIC_REGISTERS[APIC_TICR][0] = -1;
	// Now wait until PIT counter reaches zero
	while (!(__inbyte(PIT2_GATE) & 0x20));
	// Stop APIC timer
	APIC_REGISTERS[APIC_LVT0][0] = APIC_LVT_CLR;
	// Get APIC timer frequency
	DWORD freq = (-(APIC_REGISTERS[APIC_TCCR][0])) + 1;
	freq *= rate;
	QWORD shift = 1;
	while (freq >= 99)
	{
		freq++;
		freq /= 10;
		shift *= 10;
	}
	freq *= shift;
	OUTPUTTEXT(MSG0402);
	OUTPUTWORD(freq / 1000000);
	QWORD txt = 0x7A484D20;
	OUTPUTTEXT((char *) &txt);
	LINEFEED();
	// Use it as APIC timer counter initializer
	APIC_REGISTERS[APIC_TICR][0] = freq / rate;
	// Setting divide value register again not needed by the manuals
	APIC_REGISTERS[APIC_TDCR][0] = APIC_TIMER_DCR_1;
	// Finally re-enable timer in periodic mode
	APIC_REGISTERS[APIC_LVT0][0] = (IRQ_INT + APIC_TIMER_IRQ) | APIC_TIMER_MODE_PERIODIC;
}
DWORD CurrentAPIC()
{
	DWORD reg[4];
	__cpuid(reg, 1);
	if (APIC_REGISTERS)
	{
		return APIC_REGISTERS[APIC_APICID][0] >> 24;
	}
	return 0;
}
void WRICR(QWORD x)
{
	while (APIC_REGISTERS[APIC_ICRL][0] & APIC_ICR_DELIVERY_STATUS);
	APIC_REGISTERS[APIC_ICRH][0] = (x >> 32) & 0xFFFFFFFF;
	APIC_REGISTERS[APIC_ICRL][0] = (x >>  0) & 0xFFFFFFFF;
	while (APIC_REGISTERS[APIC_ICRL][0] & APIC_ICR_DELIVERY_STATUS);
}
void StartupAP(DWORD apicid)
{
	// Allocate 1*4K page for AP boot code
	QWORD pageCount = 1;
	QWORD apcode = 0;
	AllocatePhysicalMemory(&apcode, PAGE4_4K, &pageCount);
	if (apcode > 0x00100000)
	{
		OUTPUTTEXT(MSG0404);
		while (1) __halt();
	}
	// Identity mapping low 4G memory
	pageCount = 1;
	QWORD ptpd = 0;
	AllocatePhysicalMemory(&ptpd, PAGE4_4K, &pageCount);
	memset((void *) (ptpd | SYSTEM_LINEAR), 0, 0x1000);
	QWORD *L0 = (QWORD *) (__readcr3() | SYSTEM_LINEAR);
	L0[0] = ptpd | 3;
	QWORD *L1 = (QWORD *) (ptpd | SYSTEM_LINEAR);
	// Use 4*1G pages
	L1[0] = (0ULL << 30) | 0x83;
	L1[1] = (1ULL << 30) | 0x83;
	L1[2] = (2ULL << 30) | 0x83;
	L1[3] = (3ULL << 30) | 0x83;
	// Copy AP boot code
	memcpy((void *) apcode, AP_BOOT_CODE, sizeof(AP_BOOT_CODE));
	// Set code segment
	*((WORD *) (apcode + 4)) = apcode >> 4;
	// AP Stack
	QWORD stack = 0xFFFFE00000000000ULL + ((QWORD) apicid << 21) - 0x28;
	QWORD pa = 0;
	pageCount = 1;
	AllocatePhysicalMemory(&pa, PAGE4_4K, &pageCount);
	linear_mapping(pa, stack, PAGE4_4K, 7);

	*((QWORD *) (apcode + 0xC1)) = stack;
	*((DWORD *) (apcode + 0xCA)) = apicid;
	*((QWORD *) (apcode + 0xD4)) = (QWORD) APMainStartup;
	
	CORE_LOCK = 1;
	// INIT
	WRICR(APIC_ICR_LEVEL_ASSERT | APIC_ICR_DELIVERY_INIT | (((QWORD) apicid) << 56));
	// Delay 50ms
	__halt();
	// SIPI
	WRICR(APIC_ICR_LEVEL_ASSERT | APIC_ICR_DELIVERY_STARTUP | ((apcode >> 12) & 0xFF) | (((QWORD) apicid) << 56));
	while (CORE_LOCK);
	L0[0] = 0;
	FreePhysicalMemory(ptpd, PAGE4_4K, 1);
	FreePhysicalMemory(apcode, PAGE4_4K, 1);
}
void ConfigureAPIC()
{
	// Set TPR to 0, receive all interrupts
	APIC_REGISTERS[APIC_TPR][0] = 0;
	// Set DFR all bits to 1 for use flat model
	APIC_REGISTERS[APIC_DFR][0] = 0xFFFFFFFF;

	APIC_REGISTERS[APIC_LDR][0] = (APIC_REGISTERS[APIC_LDR][0] & 0x00FFFFFF) | 1;

	APIC_REGISTERS[APIC_CMCI][0] = (1 << 17);
	// Clear all lvt
	APIC_REGISTERS[APIC_LVT0][0] = APIC_LVT_CLR;
	APIC_REGISTERS[APIC_LVT1][0] = (1 << 17);
	APIC_REGISTERS[APIC_LVT2][0] = APIC_LVT_NMI;
	APIC_REGISTERS[APIC_LVT3][0] = (1 << 17);
	APIC_REGISTERS[APIC_LVT4][0] = (1 << 17);

	// Software enable APIC
	// Set the Spurious Interrupt Vector Register bit 8 to start receiving interrupts
	APIC_REGISTERS[APIC_SIV][0] = APIC_SPURIOUS_VECTOR | APIC_SOFTWARE_ENABLE;
}
void InterruptIPI(DWORD apicid, BYTE intr)
{
	WRICR(((QWORD) apicid << 56) | APIC_ICR_LEVEL_ASSERT | intr);
}
