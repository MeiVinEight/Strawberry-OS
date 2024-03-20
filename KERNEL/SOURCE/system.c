#include <system.h>
#include <declspec.h>
#include <intrinsic.h>
#include <console/console.h>
#include <memory/page.h>
#include <console/console.h>

CODEDECL OS_SYSTEM_TABLE OST;

void setup_system_table(OS_SYSTEM_TABLE * table)
{
	memcpy(&OST, table, sizeof(OS_SYSTEM_TABLE));
	memcpy(PTM, (void *) OST.PTME, PAGE_COUNT >> 3);
	PAGING = (QWORD(*)[512]) OST.PAGE;
	memcpy(&SCREEN, (void *) OST.SCRN, sizeof(CONSOLE_SCREEN));
}