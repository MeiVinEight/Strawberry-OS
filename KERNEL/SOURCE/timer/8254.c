#include <timer/8254.h>
#include <interrupt/interrupt.h>
#include <intrinsic.h>
#include <console/console.h>
#include <declspec.h>

CODEDECL const char MSG0200[] = "SETUP 8254 TIMER ";
CODEDECL const char MSG0201[] = " Hz\n";
CODEDECL const char ERR0200[] = "PIT RATE TOO SMALL\n";

void setup_8254(DWORD rate)
{
	OUTPUTTEXT(MSG0200);
	OUTPUTWORD(rate);
	OUTPUTTEXT(MSG0201);
	/*
	 * Command register:0x43
	 * Bits         Usage
	 * 6 and 7      Select channel :
	 *                 0 0 = Channel 0
	 *                 0 1 = Channel 1
	 *                 1 0 = Channel 2
	 *                 1 1 = Read-back command (8254 only)
	 * 4 and 5      Access mode :
	 *                 0 0 = Latch count value command
	 *                 0 1 = Access mode: lobyte only
	 *                 1 0 = Access mode: hibyte only
	 *                 1 1 = Access mode: lobyte/hibyte
	 * 1 to 3       Operating mode :
	 *                 0 0 0 = Mode 0 (interrupt on terminal count)
	 *                 0 0 1 = Mode 1 (hardware re-triggerable one-shot)
	 *                 0 1 0 = Mode 2 (rate generator)
	 *                 0 1 1 = Mode 3 (square wave generator)
	 *                 1 0 0 = Mode 4 (software triggered strobe)
	 *                 1 0 1 = Mode 5 (hardware triggered strobe)
	 *                 1 1 0 = Mode 2 (rate generator, same as 010b)
	 *                 1 1 1 = Mode 3 (square wave generator, same as 011b)
	 * 0            BCD/Binary mode: 0 = 16-bit binary, 1 = four-digit BCD
	 */
	 // __cli();
	DWORD divider = PIT_FREQUENCY / rate;
	if (divider >> 16)
	{
		BYTE color = SCREEN.CLR;
		SCREEN.CLR = 0x0C;
		OUTPUTTEXT(ERR0200);
		SCREEN.CLR = color;
	}
	// 0 0: Channel 0
	// 1 1: lb/hb
	// 0 1 1: square wave generator
	// 0: 16-bit binary
	__outbyte(PIT_CMD, 0x36);
	BYTE l = (divider >> 0) & 0xFF;
	BYTE h = (divider >> 8) & 0xFF;
	__outbyte(PIT0_DATA, l);
	__outbyte(PIT0_DATA, h);
	// __sti();
}
