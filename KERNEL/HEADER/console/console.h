#ifndef __KERNEL_CONSOLE_H__
#define __KERNEL_CONSOLE_H__

#include <types.h>

typedef struct _CONSOLE_SCREEN
{
	QWORD A0;  // Screen buffer address
	QWORD A1;  // Background buffer address
	WORD  H;   // Screen width
	WORD  V;   // Screen height
	WORD  ROW; // Number of row for text mode screen
	WORD  CLM; // Number of column for text mode screen
	DWORD CSR; // Cursor position
	BYTE  DM;  // Display mode, TEXT_MODE(0), GRAPHICS_MODE(1)
	BYTE  CM;  // Color mode, PALETTE_MODE(0), COLOR_MODE(1)
	BYTE  CLR; // Current 4-bit palette color
} CONSOLE_SCREEN;

extern CONSOLE_SCREEN SCREEN;
extern BYTE FONT[][16];

void SCROLLSCREEN();
void MOVECURSOR();
void CARRIAGERETURN();
void LINEFEED();
void OUTCHAR(char);
void OUTPUTTEXT(const char*);
void PRINTRAX(QWORD, BYTE);
void OUTPUTWORD(QWORD);
void setup_screen();

#endif