#include <console/console.h>
#include <declspec.h>
#include <intrinsic.h>
#include <system.h>
#include <memory/heap.h>

typedef struct _TEXT_MODE_MEMORY
{
	BYTE *TEXT;
	DWORD CURSOR;
} TEXT_MODE_MEMORY;

CODEDECL const char HEXDIG[] = "0123456789ABCDEF";
CODEDECL CONSOLE_SCREEN SCREEN;
CODEDECL TEXT_MODE_MEMORY TEXT_MODE;
CODEDECL const DWORD COLOR_PALETTE[] =
{
	0x000000,
	0x0000AA,
	0x00AA00,
	0x00AAAA,
	0xAA0000,
	0xAA00AA,
	0xFFAA00,
	0xAAAAAA,
	0x555555,
	0x5555FF,
	0x55FF55,
	0x55FFFF,
	0xFF5555,
	0xFF55FF,
	0xFFFF55,
	0xFFFFFF
};
CODEDECL BYTE(*FONT)[16];

void PAINTCURSOR(DWORD cursor)
{
	DWORD i = cursor / SCREEN.CLM;
	DWORD j = cursor % SCREEN.CLM;
	DWORD *pos = (DWORD *) SCREEN.A0;
	pos += SCREEN.H * i * 16;
	pos += j * 8;
	for (DWORD j = 0; j < 16; j++)
	{
		pos[0] = pos[1] = COLOR_PALETTE[SCREEN.CLR & 0xF];
		pos += SCREEN.H;
	}
}
void PAINTCHAR(BYTE x, BYTE c, DWORD cursor)
{
	DWORD i = cursor / SCREEN.CLM;
	DWORD j = cursor % SCREEN.CLM;
	DWORD *pos = (DWORD *) SCREEN.A0;
	pos += SCREEN.H * i * 16;
	pos += j * 8;
	BYTE *font = FONT[x];
	for (DWORD k = 0; k < 16; k++)
	{
		BYTE bit = font[k];
		for (DWORD p = 0; p < 8; p++)
		{
			if (bit & 0x80)
			{
				pos[p] = COLOR_PALETTE[(c >> 0) & 0xF];
			}
			else
			{
				pos[p] = COLOR_PALETTE[(c >> 4) & 0xF];
			}
			bit <<= 1;
		}
		pos += SCREEN.H;
	}
}
void SCROLLSCREEN()
{
	// __cli();
	memcpy(TEXT_MODE.TEXT, TEXT_MODE.TEXT + (SCREEN.CLM * 2), (SCREEN.CLM * (SCREEN.ROW - 1)) * 2);
	memset(TEXT_MODE.TEXT + (SCREEN.CLM * (SCREEN.ROW - 1)) * 2, 0, SCREEN.CLM * 2);
	SCREEN.CSR -= SCREEN.CLM;
	if (SCREEN.DM)
	{
		memcpyq((void *) SCREEN.A0, (void *) (SCREEN.A0 + (SCREEN.H * 16 * 4)), (SCREEN.H * 8 * (SCREEN.ROW - 1)));
		memsetq((void *) (SCREEN.A0 + (SCREEN.H * 16 * (SCREEN.ROW - 1) * 4)), 0, SCREEN.H * 8);
		TEXT_MODE.CURSOR -= SCREEN.CLM;
	}
	// __sti();
}
void MOVECURSOR()
{
	if (SCREEN.CSR >= SCREEN.ROW * SCREEN.CLM)
	{
		SCROLLSCREEN();
	}
	if (SCREEN.DM)
	{
		BYTE *ch = ((BYTE(*)[2]) TEXT_MODE.TEXT)[TEXT_MODE.CURSOR];
		PAINTCHAR(ch[0], ch[1], TEXT_MODE.CURSOR);
		PAINTCURSOR(TEXT_MODE.CURSOR = SCREEN.CSR);
	}
	else
	{
		// 0x3D5:0x0E high 8 bit
		__outbyte(0x03D4, 0x0E);
		__outbyte(0x03D5, SCREEN.CSR >> 8);
		// 0x3D5:0x0F low 8 bit
		__outbyte(0x03D4, 0x0F);
		__outbyte(0x03D5, SCREEN.CSR & 0xFF);
	}
}
void OUTPUTTEXT(const char *s)
{
	while (*s)
	{
		char x = *s++;
		switch (x)
		{
			case '\r':
			{
				SCREEN.CSR -= SCREEN.CSR % (SCREEN.CLM);
				MOVECURSOR();
				break;
			}
			case '\n':
			{
				SCREEN.CSR -= SCREEN.CSR % (SCREEN.CLM);
				SCREEN.CSR += SCREEN.CLM;
				MOVECURSOR();
				break;
			}
			default:
			{
				((WORD *) TEXT_MODE.TEXT)[SCREEN.CSR] = (WORD) x | (SCREEN.CLR << 8);
				SCREEN.CSR++;
				MOVECURSOR();
				break;
			}
		}
	}
}
void OUTCHAR(char x)
{
	DWORD y = (BYTE) x;
	OUTPUTTEXT((char *) &y);
}
void CARRIAGERETURN()
{
	OUTCHAR('\r');
}
void LINEFEED()
{
	OUTCHAR('\n');
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
void setup_screen()
{
	FONT = (BYTE(*)[16]) HeapAlloc(HEAPK, 4096);
	memcpy(FONT, (void *) SYSTEM_TABLE.FONT, 4096);
	SCREEN.CSR = 0;
	SCREEN.CLR = 0x0F;
	TEXT_MODE.TEXT = (BYTE *) SCREEN.A0;
	if (SCREEN.DM)
	{
		TEXT_MODE.TEXT = (BYTE *) HeapAlloc(HEAPK, SCREEN.CLM * SCREEN.ROW * 2);
		memset((void *) SCREEN.A0, 0, SCREEN.H * SCREEN.V * 4);
	}
	memset(TEXT_MODE.TEXT, 0, SCREEN.CLM * SCREEN.ROW * 2);
	TEXT_MODE.CURSOR = 0;
}