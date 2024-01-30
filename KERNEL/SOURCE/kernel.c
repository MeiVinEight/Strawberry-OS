#include <console/console.h>

void _DllMainCRTStartup()
{
	screen.color = 0x0F;
	OUTPUTTEXT("Strawberry-OS");
	while (1);
}