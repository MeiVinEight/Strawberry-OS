CODE SEGMENT
	ORG 7C00H
	START PROC
	;SET SEGMENT REGISTER TO 0
	XOR AX, AX
	MOV DS, AX
	MOV ES, AX
	MOV SS, AX
	;USE MEMORY BEFORE 7C00 AS STACK
	MOV SP, 7C00H

	;LOAD PRINT AT 0000:0400
	CLD
	MOV SI, OFFSET PRINT
	MOV DI, 0400H
	MOV CX, OFFSET PRINT02 - OFFSET PRINT
	REP MOVSB

	;USE 0000:0400 AS 21H INT
	XOR AX, AX
	MOV WORD PTR DS:[84H], 0400H
	MOV WORD PTR DS:[86H], AX

	;PRINT MESSAGE
	;MOV SI, OFFSET OSNAME
	;INT 21H

	;- - - - - - - - - - - - - - - - - - - - - - - - - - -

	;READ GPT BASE DATA (LBA 1)
	MOV DI, 8000H
	MOV SI, OFFSET DAPACK
	MOV WORD PTR [SI+08H], 01H ; LBA
	MOV AH, 42H
	INT 13H

	;READ PART TABLE LBA (+48H) TO DAPACK
	DB 66H
	MOV CX, [DI+48H]
	DB 66H
	MOV [SI+08H], CX


	;FOREACH PART TABLE
	;FIND ESP PART
	;ESP PART GUID:{C12A7328-F81F-11D2-BA4B-00A0C93EC93B}
	;USE CX AS INDX OF PART TABLE ENTRY
	XOR CX, CX

	START00:
	;IF CX IS 80H, ALL ENTRIES HAVE BEEN CHECKED
	;IT MEANS NO ESP PART IN DISK
	CMP CX, 80H
	JZ SELFSPIN

	;Read one sector for every 4 entries
	TEST CX, 03H
	JNZ START01

	MOV AH, 42H
	INT 13H
	;Increment LBA
	DB 66H
	INC WORD PTR [SI+08H]
	;Reset DI to the start of sector
	MOV DI, 8000H

	START01:
	;EFI system partition GUID first 2 byte is 7328H
	CMP WORD PTR [DI], 7328H
	JZ START04
	;GUID equals 00 means no more parts
	CMP WORD PTR [DI], 00H
	JZ SELFSPIN

	;Increment DI by 80H, skip one entry
	ADD DI, 80H
	;Increment entry index
	INC CX
	JMP START00


	;- - - - - - - - - - - - - - - - - - - - - - - - - - -


	START04:
	;DI is ESP part entry
	;[DI+20H] is EFI part start LBA
	;Read first LBA of part to 0000:8000
	DB 66H
	MOV CX, [DI+20H]
	DB 66H
	MOV [SI+08H], CX
	MOV AH, 42H
	INT 13H

	;Store FAT LBA at +00H
	MOV DI, 8000H
	DB 66H
	ADD CX, 20H
	DB 66H
	MOV [DI+00H], CX

	;Store file data LBA=part LBA + 20H + 2 * FAT sector at +04H
	DB 66H
	ADD CX, [DI+24H]
	DB 66H
	ADD CX, [DI+24H]
	DB 66H
	MOV [DI+04H], CX

	;- - - - - - - - - - - - - - - - - - - - - - - - - - -

	;Read first sector of file data into 0000:6000
	MOV WORD PTR [SI+04H], 6000H
	DB 66H
	MOV [SI+08H], CX
	MOV AH, 42H
	INT 13H

	;Find file in file records
	;Save SI in stack
	PUSH SI
	MOV SI, 6000H
	MOV DI, OFFSET FIL0
	CALL FINDFILE
	;Restore SI
	POP SI

	;- - - - - - - - - - - - - - - - - - - - - - - - - - -

	DB 66H
	MOV BP, AX
	;Read FAT1 LBA from 0000:8000
	MOV DI, 8000H
	DB 66H
	MOV CX, [DI]
	;Read FAT1 into 0000:6000
	DB 66H
	MOV [SI+08H], CX
	MOV AH, 42H
	INT 13H

	;- - - - - - - - - - - - - - - - - - - - - - - - - - -

	;SHL 2 equals MUL 4
	MOV CL, 02H
	;Read STBRBOOT into 1000:0000 or 00010000 (32bit)
	MOV WORD PTR [SI+04H], 00H
	MOV WORD PTR [SI+06H], 1000H

	START07:
	;BP=FFFFH (EBP=0FFFFFFFH) means no more data
	CMP BP, 0FFFFH
	JZ START08

	;Calculate data sector LBA: cluster-2+(file data LBA)
	DB 66H
	MOV AX, BP
	DB 66H
	SUB AX, 02H
	;file data LBA was stored in 8004H
	MOV DI, 8004H
	DB 66H
	ADD AX, [DI]
	;Read sector
	DB 66H
	MOV [SI+08H], AX
	MOV AH, 42H
	INT 13H
	;Increment 0200H per sector
	ADD WORD PTR [SI+04H], 0200H

	;Multiply EBP by 2
	DB 66H
	SHL BP, CL
	;Read next cluster
	DB 66H
	MOV BP, [BP+6000H]
	JMP START07

	;- - - - - - - - - - - - - - - - - - - - - - - - - - -

	START08:
	; JMP FAR 1000H:0000H
	DB 0EAH
	DW 0000H, 1000H

	;ENTER ENDLESS LOOP
	SELFSPIN:
	JMP SELFSPIN

	;-----------------------------------------------------
	; print text to console/screen
	; DS:SI null terminated ascii string
	PRINT:; PROC
	PUSH AX
	PUSH BX
	PUSH SI
	MOV AH, 0EH; OUTPUT CHAR
	MOV BX, 00H; 0 PAGE 0 COLOR

	PRINT00:
	MOV AL, BYTE PTR DS:[SI]
	TEST AL, AL; BREAK IF \0
	JZ SHORT PRINT01

	INT 10H
	INC SI
	JMP PRINT00

	PRINT01:
	POP SI
	POP BX
	POP AX
	IRET

	PRINT02:
	START ENDP


	; -----------------------------------------------------
	;Find file named in DI from sector data in SI
	;Return cluster number in EAX
	;Returns EAX = 0 means not found
	FINDFILE PROC
	PUSH CX
	PUSH BP
	PUSH SI
	;10H file entries per sector
	MOV CX, 10H

	FINDFILE00:
	;All entries have been checked
	;No such file or folder
	TEST CX, CX
	JZ FINDFILE04

	;00H-0AH is 8.3 file name format
	;0BH is attribute, 10H means this is a folder, 20H means this is a file
	MOV BP, 0CH

	;Compare file name and attribute
	FINDFILE01:
	TEST BP, BP
	JZ FINDFILE03

	;Compare 2 bytes at a time
	SUB BP, 02H
	MOV AX, [SI+BP]
	CMP AX, [DI+BP]
	JNZ FINDFILE02
	JMP FINDFILE01

	;Not equal
	FINDFILE02:
	;Decrement dir entry count
	DEC CX
	;20H per dir entry
	ADD SI, 20H
	JMP FINDFILE00

	;Equal, found
	FINDFILE03:
	;+14H is high 2 byte of file cluster
	MOV CL, 10H
	MOV AX, [SI+14H]
	DB 66H
	SHL AX, CL
	;+1AH is low 2 byte of file cluster
	MOV AX, [SI+1AH]
	JMP FINDFILE05

	; Not found, return 0
	FINDFILE04:
	DB 66H
	XOR AX, AX

	FINDFILE05:
	POP SI
	POP BP
	POP CX
	RET
	FINDFILE ENDP

	OSNAME DB "STRAWBERRY OS", 0
	HEXDIG DB '0123456789ABCDEF'
	DAPACK DW 10H  ; 00H: packte size
		   DW 01H  ; 02H: block count
		   DW 8000H; 04H: buffer address
		   DW 00H  ; 06H: buffer segment
		   DQ 00H  ; 08H: LBA
	FIL0   DB 'STBRBOOT   ', 20H
CODE ENDS
END START