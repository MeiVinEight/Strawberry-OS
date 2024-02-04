#include <console/console.h>
#include <declspec.h>

CODEDECL const char HEXDIG[] = "0123456789ABCDEF";

void PRINTRAX(QWORD x, BYTE s)
{
	char buf[17];
	buf[s] = 0;
	while (s--)
	{
		buf[s] = HEXDIG[x & 0xF];
		x >>= 4;
	}
	OUTPUTTEXT(buf);
}