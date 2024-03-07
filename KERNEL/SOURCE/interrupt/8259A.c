#include <interrupt/interrupt.h>
#include <interrupt/8259A.h>
#include <intrinsic.h>
#include <console/console.h>
#include <declspec.h>

CODEDECL const char MSG0300[] = "SETUP 8259A\n";
CODEDECL const char MSG0301[] = "DISABLE 8259A\n";

void setup_8259A()
{
	OUTPUTTEXT(MSG0300);
	// 8259A EOI
	interrupt_eoi = eoi_8259A;
	// Master PIC
	__outbyte(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); // Start init sequence (in cascade mode)
	__outbyte(PIC1_DATA, PIC1_OFFSET); // ICW2: Master PIC vector offset: 0x20
	__outbyte(PIC1_DATA, 4); // ICW3: Tell master PIC that is a slave PIC at IRQ2 (0000 0[1]00)
	__outbyte(PIC1_DATA, ICW4_8086); // ICW4: Have the PICs use 8086 mode
	// Slave PIC
	__outbyte(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4); // Start init sequence (in cascade mode)
	__outbyte(PIC2_DATA, PIC2_OFFSET); // ICW2: Slave PIC vector offset: 0x28
	__outbyte(PIC2_DATA, 2); // ICW3: Tell slave PIC its cascade identity (2)
	__outbyte(PIC2_DATA, ICW4_8086); // ICW4: Have the PICs use 8086 mode
}
void eoi_8259A(BYTE id)
{
	if (id < 0x30)
	{
		if (id > 0x27)
		{
			__outbyte(PIC2_COMMAND, ICW0_EOI);
		}
		__outbyte(PIC1_COMMAND, ICW0_EOI);
	}
}
void disable_8259A()
{
	OUTPUTTEXT(MSG0301);
	__outbyte(PIC1_DATA, ICW1_DISABLE);
	__outbyte(PIC2_DATA, ICW1_DISABLE);
}