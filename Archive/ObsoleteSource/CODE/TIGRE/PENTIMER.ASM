;
;	PENTIMER.ASM
;       latch the pentium timer
;       puts hi 34 bits at [bx] and low 34 bits at [cx]
;
;	June 26th, 1996	Van
;	(c) Copyright 1996, Tachyon, Inc.  All rights reserved.
;----[]-------------------------------------------------------------
.486
DGROUP	GROUP	_OSCODE

_OSDATA	SEGMENT	PARA PUBLIC USE32 'DATA'
_OSDATA	ENDS

_OSCODE	SEGMENT 	PARA PUBLIC USE32 'CODE'
			ASSUME	DS:DGROUP
			ASSUME  	CS:_OSCODE

			PUBLIC			SetXMode_

;---[ ReadPentiumTimer ]---------------------------------------------
;
; Reads count of clock cycles spent since boot.
;
; Parameters:		None
; Returns:		lower 32 bits of 64-bit timer
;
;--------------------------------------------------------------------
ReadPentiumTimer_	proc

	pushf		; save flags
	push	edx	; save edx (dont worry about eax)

        DB  0fh, 031h	; latch and read the cycle counter
        mov [eax], edx	; store lower 32-bits for return value
        pop	edx	; restore EDX
	popf		; and flags
        ret

ReadPentiumTimer_	endp

