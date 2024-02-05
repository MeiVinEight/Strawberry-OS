#include <timer/timer.h>
#include <timer/8254.h>
#include <interrupt/interrupt.h>
#include <interrupt/apic.h>
#include <console/console.h>

void interrupt_timer(INTERRUPT_STACK* stack)
{
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
}