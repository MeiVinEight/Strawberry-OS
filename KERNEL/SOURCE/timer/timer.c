#include <timer/timer.h>
#include <timer/8254.h>

void setup_timer()
{
	setup_8254(20); // 20 Hz timer
}