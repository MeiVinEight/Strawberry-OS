#include <interrupt/apic.h>
#include <declspec.h>
#include <types.h>
#include <intrinsic.h>
#include <console/console.h>
#include <interrupt/8259A.h>
#include <interrupt/interrupt.h>
#include <timer/8254.h>
#include <timer/timer.h>

#define CPUID_FEAT_EDX_APIC (1 << 9)

#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_BSP 0x100 // Processor is a BSP
#define IA32_APIC_BASE_MSR_ENABLE 0x800

#define APIC_SOFTWARE_ENABLE 0x100
#define APIC_SPURIOUS_VECTOR 0xFF

#define APIC_APICID  0x002
#define APIC_APICVER 0x003
#define APIC_TPR     0x008
#define APIC_APR     0x009
#define APIC_PPR     0x00A
#define APIC_EOI     0x00B
#define APIC_RRD     0x00C
#define APIC_LDR     0x00D
#define APIC_DFR     0x00E
#define APIC_SIV     0x00F
#define APIC_ISR     0x010
#define APIC_TMR     0x018
#define APIC_IRR     0x020
#define APIC_ESR     0x028
#define APIC_CMCI    0x02F
#define APIC_ICRL    0x030
#define APIC_ICRH    0x031
#define APIC_LVT0    0x032
#define APIC_LVT1    0x033
#define APIC_LVT2    0x034
#define APIC_LVT3    0x035
#define APIC_LVT4    0x036
#define APIC_LVTE    0x037
#define APIC_ICR     0x038
#define APIC_CCR     0x039
#define APIC_DCR     0x03E

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
CODEDECL const char MSG0402[] = "APIC TIMER FREQUENCY=";
CODEDECL const QWORD APIC_BASE_ADDRESS = 0x00100000;
CODEDECL DWORD (*APIC_REGISTERS)[4];

void set_apic_address(QWORD adrs)
{
	DWORD eax = (adrs & 0xFFFFF000);
	__writemsr(IA32_APIC_BASE_MSR, eax);
}
void eoi_apic()
{
	APIC_REGISTERS[APIC_EOI][0] = 0;
}

int check_apic()
{
	DWORD cpui[4] = {0};
	__cpuid(cpui, 1);
	return !!(cpui[3] & CPUID_FEAT_EDX_APIC);
}
void setup_apic()
{
	disable_8259A();

	OUTPUTTEXT(MSG0400);
	set_apic_address(APIC_BASE_ADDRESS);
	APIC_REGISTERS = (DWORD (*)[4]) APIC_BASE_ADDRESS;
	// Set TPR to 0, receive all interrupts
	APIC_REGISTERS[APIC_TPR][0] = 0;
	// Set DFR all bits to 1 for use flat model
	APIC_REGISTERS[APIC_DFR][0] = 0xFFFFFFFF;

	APIC_REGISTERS[APIC_LDR][0] = (APIC_REGISTERS[APIC_LDR][0] & 0x00FFFFFF) | 1;

	// Clear all lvt
	APIC_REGISTERS[APIC_LVT0][0] = APIC_LVT_CLR;
	APIC_REGISTERS[APIC_LVT1][0] = APIC_LVT_CLR;
	APIC_REGISTERS[APIC_LVT2][0] = APIC_LVT_NMI;
	APIC_REGISTERS[APIC_LVT3][0] = APIC_LVT_CLR;
	APIC_REGISTERS[APIC_LVT4][0] = APIC_LVT_CLR;
	
	// Enable APIC
	__writemsr(IA32_APIC_BASE_MSR, __readmsr(IA32_APIC_BASE_MSR) | IA32_APIC_BASE_MSR_ENABLE);

	// Software enable APIC
	// Set the Spurious Interrupt Vector Register bit 8 to start receiving interrupts
	APIC_REGISTERS[APIC_SIV][0] = APIC_SPURIOUS_VECTOR | APIC_SOFTWARE_ENABLE;
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
	APIC_REGISTERS[APIC_DCR][0] = 11;

	// Prepare PIT sleep 50 ms
	__outbyte(PIT2_GATE, (__inbyte(PIT2_GATE) & 0xFD) | 1);
	__outbyte(PIT_CMD, 0B10110010);
	DWORD frq = PIT_FREQUENCY / rate;
	__outbyte(PIT2_DATA, (frq >> 0) & 0xFF);
	__inbyte(0x60);
	__outbyte(PIT2_DATA, (frq >> 8) & 0xFF);
	// Reset PIT one-shot counter (start counting)
	BYTE al = __inbyte(PIT2_GATE) & 0xFE;
	__outbyte(PIT2_GATE, al);
	__outbyte(PIT2_GATE, al | 1);
	// Reset APIC timer (set counter to -1)
	APIC_REGISTERS[APIC_ICR][0] = -1;
	// Now wait until PIT counter reaches zero
	while (!(__inbyte(PIT2_GATE) & 0x20));
	// Stop APIC timer
	APIC_REGISTERS[APIC_LVT0][0] = APIC_LVT_CLR;
	// Get APIC timer frequency
	DWORD freq = (-(APIC_REGISTERS[APIC_CCR][0])) + 1;
	OUTPUTTEXT(MSG0402);
	OUTPUTWORD(freq * rate);
	OUTPUTTEXT(text);
	LINEFEED();
	// Use it as APIC timer counter initializer
	APIC_REGISTERS[APIC_ICR][0] = freq;
	// Setting divide value register again not needed by the manuals
	APIC_REGISTERS[APIC_DCR][0] = 11;
	// Finally re-enable timer in periodic mode
	APIC_REGISTERS[APIC_LVT0][0] = (IRQ_INT + APIC_TIMER_IRQ) | APIC_TIMER_MODE_PERIODIC;
	// APIC EOI
	interrupt_eoi = eoi_apic;
}