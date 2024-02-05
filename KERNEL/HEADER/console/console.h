#ifndef __KERNEL_CONSOLE_H__
#define __KERNEL_CONSOLE_H__

#include <types.h>

typedef struct _VGA_SCREEN
{
	WORD cursor;
	BYTE color;
} VGA_SCREEN;

extern VGA_SCREEN screen;

void SCROLLSCREEN();
void MOVECURSOR();
void CARRIAGERETURN();
void LINEFEED();
void OUTCHAR(char);
void OUTPUTTEXT(const char*);
void PRINTRAX(QWORD, BYTE);
void OUTPUTWORD(QWORD);

#endif