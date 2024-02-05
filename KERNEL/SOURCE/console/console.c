#include <console/console.h>
#include <declspec.h>

CODEDECL const char HEXDIG[] = "0123456789ABCDEF";

void OUTPUTTEXT(const char* s)
{
	while (*s)
	{
		OUTCHAR(*s++);
	}
}
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
void OUTPUTWORD(QWORD x)
{
	if (x)
	{
		char buf[33];
		buf[32] = 0;
		DWORD idx = 32;
		while (x)
		{
			buf[--idx] = (x % 10) + '0';
			x /= 10;
		}
		OUTPUTTEXT(buf + idx);
		return;
	}
	OUTCHAR('0');
}