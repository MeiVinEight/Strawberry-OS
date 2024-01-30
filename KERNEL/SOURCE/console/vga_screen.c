#include <console/console.h>
#include <declspec.h>

CODEDECL VGA_SCREEN screen =
{
	.cursor = 0,
	.color = 0x07
};


void MOVECURSOR()
{
	if (screen.cursor >= 0x07D0)
	{
		screen.cursor -= 0x50;
		SCROLLSCREEN();
	}
	// 0x3D5:0x0E high 8 bit
	__outbyte(0x03D4, 0x0E);
	__outbyte(0x03D5, screen.cursor >> 8);
	// 0x3D5:0x0F low 8 bit
	__outbyte(0x03D4, 0x0F);
	__outbyte(0x03D5, screen.cursor & 0xFF);
}
void CARRIAGERETURN()
{
	screen.cursor -= (screen.cursor % 0x50);
	MOVECURSOR();
}
void LINEFEED()
{
	// \r
	CARRIAGERETURN();
	// \n:move cursor to next line: cursor + 0x50
	screen.cursor += 0x50;
	MOVECURSOR();
}
void OUTPUTTEXT(const char* s)
{
	while (*s)
	{
		// low 8 bit is ASCII code
		WORD x = *s;
		if (x == '\r')
		{
			CARRIAGERETURN();
		}
		else if (x == '\n')
		{
			LINEFEED();
		}
		else
		{
			// not CR or LF, output ascii code
			// high 8 bit is text attribute
			x |= screen.color << 8;
			// write to cursor position
			*((WORD*)(0x000B8000ULL + ((QWORD) screen.cursor << 1))) = x;
			// Increment cursor pos
			screen.cursor++;
		}
		s++;
		MOVECURSOR();
	}
}