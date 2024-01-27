CODE SEGMENT
	START PROC
	;Init segment registers to 1000H
	MOV AX, 1000H
	MOV DS, AX
	MOV ES, AX
	;Output message
	;MOV SI, OFFSET OSNAME
	;INT 21H

	;- - - - - - - - - - - - - - - - - - - - - - - - - - -

	;1. Set GDT at 0000:0420
	;GDT[0] = 0x0000000000000000 RSV
	;GDT[1] = 0x0020980000000000 CODE
	;GDT[2] = 0x0000920000000000 DATA
	DB 66H
	XOR AX, AX
	DB 66H
	XOR CX, CX
	MOV DS, AX
	MOV SI, 0420H
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

	;2. Set Page Table at 0x00001000
	;Clear 5*4KB memory for Page Table
	XOR AX, AX
	MOV ES, AX
	MOV DI, 1000H
	MOV CX, 2800H; 5*4K has 2800H WORD(s)
	CLD
	REP STOSW
	;1-level page table has 2 entry
	;Point to 2 2-level page table
	MOV DI, 1000H
	MOV WORD PTR [DI+0000H], 2003H; 0x00002000, 1-LVL EBTRY 0x000
	MOV WORD PTR [DI+0800H], 3003H; 0x00003000, 1-LVL ENTRY 0x100
	;2-level page table(1) has 1 entry, mapping 0x0000000000000000
	;Point to 3-level page table
	MOV DI, 2000H
	MOV WORD PTR [DI+0000H], 4003H; 0x00004000
	;2-level page table(2) has 1 entry, mapping 0xFFFF800000000000
	;Point to 3-level page table
	MOV DI, 3000H
	MOV WORD PTR [DI+0000H], 5003H; 0x00004000
	;3-level page table(1) has 1 entry, mapping 0x0000000000000000
	;Point to 0x00000000
	MOV DI, 4000H
	MOV WORD PTR [DI+0000H], 0183H; 0x00000000, Global Page
	;3-level page table(2) has 1 entry, mapping 0xFFFF800000000000
	;Point to 0x00200000
	MOV DI, 5000H
	MOV WORD PTR [DI+0000H], 0183H; 0x00200000, Global Page
	MOV WORD PTR [DI+0002H], 0020H

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
	MOV AX, 1000H; PHY ADDR 0x00001000
	DW 0000H
	;MOV CR3, EAX
	DB 66H, 0FH, 22H, 18H

	;- - - - - - - - - - - - - - - - - - - - - - - - - - -

	;7. Load GDT address to GDTR
	DB 66H
	XOR CX, CX
	DB 66H
	PUSH CX; GDT physical address high 32 bit

	DB 66H
	MOV SI, 0420H
	DW 00H
	DB 66H
	PUSH SI; GDT physical address low 32 bit

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
	DD 00010100H
	DW 0008H

	;- - - - - - - - - - - - - - - - - - - - - - - - - - -

	SELFSPIN:
	JMP SELFSPIN
	START ENDP
	;OSNAME DB 'STRAWBERRY OS', 0

	ORG 100H
	LONGMODE PROC
	MOV AX, 8000H
	DW 000BH
	;MOV WORD PTR [RAX], 5307H
	DB 66H, 0C7H, 00H, 53H, 07H
	SELFSPIN:
	JMP SELFSPIN
	LONGMODE ENDP
CODE ENDS
END START