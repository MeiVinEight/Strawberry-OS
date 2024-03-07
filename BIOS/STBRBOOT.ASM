CODE SEGMENT
	START PROC

	JMP SHORT STBRBOOT
	ALIGN 08H
	DQ 2FH
	;DISK GUID = {BE559BDA-5715-4BAB-89D3-66D22BF6A8B6}
	DQ 4BAB5715BE559BDAH, 0B6A8F62BD266D389H
	;PART GUID = {6A655705-B249-4C8C-A527-AEAAC113B6D8}
	DQ 4C8CB2496A655705H, 0D8B613C1AAAE27A5H
	;POINTER TO IMAGE
	DW OFFSET LONGMODE
	;DISPLAY MODE
	;0: TEXT MODE
	;1: GRAPHICS MODE
	DB 00H
	;COLOR MODE
	;0: 4BIT COLOR PLAETTE
	;1: 32BIT RGB
	DB 00H
	;HORIZONTAL
	DW 50H
	;VERTICAL
	DW 19H
	;MMIO ADDRESS (A0)
	DQ 000B8000H
	;ACPI RSDP ADDRESS
	DQ 00H

	STBRBOOT:
	;Init segment registers to 1000H
	MOV AX, 1000H
	MOV DS, AX
	MOV ES, AX
	;Output message
	;MOV SI, OFFSET OSNAME
	;INT 21H

	;- - - - - - - - - - - - - - - - - - - - - - - - - - -

	;Detecting Memory
	;Save at 3018H
	PUSH ES
	DB 66H
	MOV DX, 4150H
	DW 534DH
	DB 66H
	XOR BX, BX
	MOV DI, 3020H
	LEA SI, [DI-8]
	MOV ES, BX

	DETECTING_MEMORY:
	DB 66H
	MOV AX, 0E820H
	DW 00H
	DB 66H
	XOR CX, CX
	MOV CL, 18H
	INT 15H
	DB 66H
	TEST BX, BX
	JZ DETECTING_COMPLETE

	ADD DI, 18H
	JMP DETECTING_MEMORY

	DETECTING_COMPLETE:
	MOV ES:[SI], DI
	POP ES

	;- - - - - - - - - - - - - - - - - - - - - - - - - - -

	;0. Calculate PE32+ entry
	;Read Image Base
	MOV BX, OFFSET JMPENTRY
	DB 66H
	MOV SI, [BX]
	;Calculate NT Header
	DB 66H
	MOV DI, SI
	DB 66H
	ADD DI, [SI+3CH]
	;Calculate Entry Point Address
	DB 66H
	ADD SI, [DI+28H]
	;Store address to jmp
	DB 66H
	MOV [BX], SI

	;- - - - - - - - - - - - - - - - - - - - - - - - - - -

	;1. Set GDT at 0000:3000
	;GDT[0] = 0x0000000000000000 RSV
	;GDT[1] = 0x0020980000000000 CODE
	;GDT[2] = 0x0000920000000000 DATA
	DB 66H
	XOR AX, AX
	DB 66H
	XOR CX, CX
	MOV DS, AX
	MOV SI, 3000H
	;GDT[0]
	DB 66H
	MOV [SI+00H], AX
	DB 66H
	MOV [SI+04H], AX
	;GDT[1]
	DB 66H
	MOV CX, 9800H
	DW 0020H
	DB 66H
	MOV [SI+08H], AX
	DB 66H
	MOV [SI+0CH], CX
	;GDT[2]
	DB 66H
	MOV CX, 9200H
	DW 0000H
	DB 66H
	MOV [SI+10H], AX
	DB 66H
	MOV [SI+14H], CX

	;- - - - - - - - - - - - - - - - - - - - - - - - - - -

	;2. Set Page Table at 0x00005000
	;Mapping 0x00000000+2M to 0x0000000000000000
	;Clear 3*4KB memory for Page Table
	XOR AX, AX
	MOV ES, AX
	MOV DI, 5000H
	MOV CX, 1800H; 3*4K has 1800H WORD(s)
	CLD
	REP STOSW
	;1-level page table has 1 entry
	;Point to 2 2-level page table
	MOV DI, 5000H
	MOV WORD PTR ES:[DI+0000H], 6003H; 0x00006000, 1-LVL ENTRY 0x000
	;2-level page table has 1 entry, mapping 0x0000000000000000
	;Point to 3-level page table
	MOV DI, 6000H
	MOV WORD PTR ES:[DI+0000H], 7003H; 0x00007000, 2-LVL ENTRY 0x000
	;3-level page table has 1 entry, mapping 0x0000000000000000
	;Point to physical address 0x00000000
	MOV DI, 7000H
	MOV WORD PTR ES:[DI+0000H], 0183H; 0x00000000, Global Page

	;- - - - - - - - - - - - - - - - - - - - - - - - - - -

	;3. Clear interrupt
	CLI

	;- - - - - - - - - - - - - - - - - - - - - - - - - - -

	;4. OPEN A20
	IN AL, 92H
	OR AL, 02H
	OUT 92H, AL

	;- - - - - - - - - - - - - - - - - - - - - - - - - - -

	;5. Enable PAE and PGE
	;MOV EAX, CR4
	DB 66H, 0FH, 20H, 20H
	DB 66H
	OR AX, 20H; CR4.PAE
	DB 66H
	OR AX, 0080H; CR4.PGE
	DW 00H
	;MOV CR4, EAX
	DB 66H, 0FH, 22H, 20H

	;- - - - - - - - - - - - - - - - - - - - - - - - - - -

	;6. Set Page Table address CR3 bit 12:63
	DB 66H
	MOV AX, 5000H; PHY ADDR 0x00005000
	DW 0000H
	;MOV CR3, EAX
	DB 66H, 0FH, 22H, 18H

	;- - - - - - - - - - - - - - - - - - - - - - - - - - -

	;7. Load GDT address to GDTR
	DB 66H
	XOR CX, CX
	DB 66H
	PUSH CX; GDT physical address high 32 bit

	; GDT physical address low 32 bit
	MOV SI, 0000H
	PUSH SI
	MOV SI, 3000H
	PUSH SI

	MOV AX, 0018H
	PUSH AX;GDT size = 18H
	MOV SI, SP
	;LGDT [SI]
	DB 0FH, 01H, 14H
	ADD SP, 0AH
	;Set segment selector
	MOV AX, 10H
	MOV DS, AX
	MOV ES, AX
	MOV SS, AX
	;Set stack at 0x00020000
	DB 66H
	MOV SP, 00H
	DW 02H

	;- - - - - - - - - - - - - - - - - - - - - - - - - - -

	;8. Set EFER.LME to 1 (ENABLE Long Mode)
	DB 66H
	MOV CX, 0080H
	DW 0C000H
	;RDMSR     ;EFER
	DB 0FH, 32H
	DB 66H
	OR AX, 100H ;EFER.LME
	DW 00H
	;WRMSR
	DB 0FH, 30H

	;- - - - - - - - - - - - - - - - - - - - - - - - - - -

	;9. Enable CR0.PE and CR0.PG
	;MOV EAX, CR0
	DB 66H, 0FH, 20H, 00H
	;BIT  0 is CR0.PE
	;BIT 31 is CR0.PG
	;MOV ECX, 80000001H
	DB 66H
	MOV CX, 0001H
	DW 8000H
	DB 66H
	OR AX, CX
	;MOV CR0, EAX
	DB 66H, 0FH, 22H, 00H

	;- - - - - - - - - - - - - - - - - - - - - - - - - - -

	;10. JMP FAR 0008:00010200
	DB 66H, 0EAH
	JMPENTRY:
	DW OFFSET LONGMODE
	DW 0001H
	DW 0008H

	;- - - - - - - - - - - - - - - - - - - - - - - - - - -

	SELFSPIN:
	JMP SELFSPIN
	START ENDP
	;OSNAME DB 'STRAWBERRY OS', 0

	ALIGN 10H
	LONGMODE PROC
	SELFSPIN:
	JMP SELFSPIN
	LONGMODE ENDP
CODE ENDS
END START