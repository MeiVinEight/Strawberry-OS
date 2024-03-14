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

#define CPUID_FEAT_EDX_APIC (1 << 9)

#define IA32_APIC_BASE_MSR 0x1B
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
#define APIC_ICR     0x38
#define APIC_CCR     0x39
#define APIC_DCR     0x3E

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
CODEDECL const char MSG0404[] = "APIC MEMORY MAPPING FAILED\n";
CODEDECL DWORD (*APIC_REGISTERS)[4];

void set_apic_address()
{
	QWORD apic_base_msr = __readmsr(IA32_APIC_BASE_MSR);
	QWORD apic_base = apic_base_msr & (~0xFFF);
	QWORD apic_base_linear = apic_base | 0xFFFF800000000000ULL;
	// Hardware enable APIC
	// apic_base |= (1 << 11);
	// __writemsr(IA32_APIC_BASE_MSR, apic_base_msr);
	// APIC Registers base
	APIC_REGISTERS = (DWORD(*)[4]) apic_base_linear;
	if (linear_mapping(apic_base, apic_base_linear, 0))
	{
		OUTPUTTEXT(MSG0404);
		while (1) __halt();
	}
	OUTPUTTEXT(MSG0403);
	PRINTRAX(apic_base_linear, 16);
	LINEFEED();
}
void eoi_apic(BYTE id)
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
void setup_apic()
{
	disable_8259A();

	OUTPUTTEXT(MSG0400);
	set_apic_address();
	// APIC EOI
	interrupt_eoi = eoi_apic;
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
	APIC_REGISTERS[APIC_DCR][0] = APIC_TIMER_DCR_1;

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
	APIC_REGISTERS[APIC_DCR][0] = APIC_TIMER_DCR_1;
	// Finally re-enable timer in periodic mode
	APIC_REGISTERS[APIC_LVT0][0] = (IRQ_INT + APIC_TIMER_IRQ) | APIC_TIMER_MODE_PERIODIC;
}
