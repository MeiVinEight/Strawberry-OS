#include <timer/timer.h>
#include <timer/8254.h>
#include <interrupt/interrupt.h>
#include <interrupt/apic.h>
#include <console/console.h>
#include <intrinsic.h>
#include <declspec.h>
#include <device/usb/hid.h>

CODEDECL const char MSG0700[] = "INVARIANT TSC\n";
QWORD TSC_FREQUENCY_KHZ = 0;

void interrupt_timer(INTERRUPT_STACK* stack)
{
	USBKeyboardEvent();
	// Do nothing
}
void setup_timer()
{
	// 20 Hz timer
	if (USEAPIC)
	{
		setup_apic_timer(TIMER_INT_FREQUENCY);
	}
	else
	{
		setup_8254(TIMER_INT_FREQUENCY);
	}
	// Register IRQ0 interrupt
	register_interrupt(IRQ_INT, interrupt_timer);
	DWORD reg[4];
	__cpuid(reg, 0x80000007);
	if (reg[3] & (1 << 8))
	{
		OUTPUTTEXT(MSG0700);
		// Calibrate the CPU time-stamp-counter

		QWORD start = __rdtsc();
		DWORD delay = TIMER_INT_FREQUENCY;
		while (delay--) __halt();
		QWORD end = __rdtsc();

		// Store calibrated CPU KHz
		TSC_FREQUENCY_KHZ = (end - start) / 1000;
		QWORD shift = 1;
		while (TSC_FREQUENCY_KHZ >= 99)
		{
			TSC_FREQUENCY_KHZ++;
			TSC_FREQUENCY_KHZ /= 10;
			shift *= 10;
		}
		TSC_FREQUENCY_KHZ *= shift;
		QWORD txt = ' UPC';
		OUTPUTTEXT((char *) &txt);
		OUTPUTWORD(TSC_FREQUENCY_KHZ / 1000);
		txt = 0x0A7A484D20; // [ MHz\n]
		OUTPUTTEXT((char *) &txt);
	}
}
void delay(QWORD ms)
{
	// Step.1 Use timer interrupt to yield
	QWORD stop = __rdtsc() + TSC_FREQUENCY_KHZ * ms;
	QWORD unit = 1000 / TIMER_INT_FREQUENCY;
	while (((stop - __rdtsc()) / TSC_FREQUENCY_KHZ) >= unit)
	{
		__halt();
	}
	// Step.2 Use a hardware timer to wait micro time
	while (__rdtsc() < stop) __nop();
}
QWORD TimestampCPU()
{
	return __rdtsc() / (TSC_FREQUENCY_KHZ);
}