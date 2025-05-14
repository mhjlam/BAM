;
;	OSGRPH.ASM
;
;	October 5, 1994  BKH
;  Copyright (C) 1993, Tachyon, Inc
;	All rights reserved.
;
;	Assembly routines for OSGRAPH functions, including:
;
;		OS_ShutDownVideo
;		GetScreenMode
;
;		SetPalette
;		GetPalette
;
;		CopyPixelLine_ASM
;
;	Parameters are passed in the following register order:
;
;		AX, DX, BX, CX, stack
;
;	unless any of the first four parameters are too large to fit
;	in a register, in which case that parameter is put on the stack
;	and the next parameter takes its place in the register order.
;
;----[]-------------------------------------------------------------


DGROUP	GROUP		_OSDATA,_OSCODE

_OSDATA	SEGMENT	PARA PUBLIC USE32 'DATA'

			videoMode	dw		0			; Used for video modes passed in
			screenWide	dw		0			; Width of screen
			screenHigh	dw		0			; Height of screen

			gunsArray	dd		0			; Address of palette guns array
			startGun		dw		0			; Starting VGA register
			endGun		dw		0			; Ending VGA register

			ASMCP	struc
				cpSrcBuf		dd		0		; Source buffer
				cpDstBuf		dd		0		; Destination buffer
				cpCLUT		dd		0		; Address of CLUT
				cpCLUTIdx	dd		0		; Pal register # of first CLUT entry
				cpCLUTSize	dd		0		; # entries in CLUT
				cpBufWide	dd		0		; Width of source buffer
				cpSrcOffset	dd		0		; Offset into source buffer
				cpDstOffset	dd		0		; Offset into destination buffer
				cpHDir		dd		0		; Are we copying R to L or L to R?
				cpDoSkip		db		0		; Should we respect skip?
				cpSRLEFlag	db		0		; Is it an SRLE line?
			ASMCP	ends

			;;acp		dd		0			; Our ASMCP instance

			;;cpEndMark	dd		0			; How we know we're done with a row
			;;cpSkipBytes	db		0			; How many bytes of leading skip?
			;;cpIgnored	db		0			; How many pixels do we ignore?

_OSDATA	ENDS


_OSCODE	SEGMENT 	PARA PUBLIC USE32 'CODE'
			ASSUME	DS:DGROUP
			ASSUME  	CS:_OSCODE

			PUBLIC  	OS_ShutDownVideo_,
						OS_GetScreenMode_,
						SetScreenMode_,

						OS_SetPalette_,
						OS_GetPalette_,

						OS_GetStackPtr_,

						OS_CopyPixelLine_ASM_


; General equates

PALETTE_PORT		equ	03C8h
SKIP_COLOR			equ	00FEh

SET_SCREEN_MODE	equ	00h
GET_SCREEN_MODE	equ	0Fh
VIDEO_BIOS			equ	10h


;---[ OS_ShutDownVideo ]------------------------------------------------
;
; atexit function to return video to startup mode
;
; Parameters:  AX = Original mode number
;
; Stack Use:	None
;
; Returns:		Nothing
;
;--------------------------------------------------------------------

OS_ShutDownVideo_	proc

	push	eax
	call	SetScreenMode_
	pop	eax

	ret

OS_ShutDownVideo_	endp



;---[ GetScreenMode ]------------------------------------------------
;
; Get the current screen mode
;
; Parameters:	None
;
; Stack Use:	None
;
; Returns:		AX = Screen mode
;
;--------------------------------------------------------------------

OS_GetScreenMode_	proc

	mov	ah,GET_SCREEN_MODE			; AH=get screen mode BIOS function
	int	VIDEO_BIOS
	movzx	eax,al

	ret

OS_GetScreenMode_	endp



;---[ SetScreenMode ]------------------------------------------------
;
; Set the screen mode
;
; Parameters:	AX = New mode number
;
; Stack Use:	None
;
; Returns:		AX = 1 if successful, 0 if not
;
;--------------------------------------------------------------------

SetScreenMode_	proc

	mov 		videoMode,ax
	mov		ah,SET_SCREEN_MODE
	int		VIDEO_BIOS
	movzx		eax,al

	ret

SetScreenMode_	endp



;---[ SetPalette ]---------------------------------------------------
;
; Sets the VGA palette registers
;
; Parameters:	AX = pointer to character array of guns information
;					DX = starting register number
;					BX = ending register number
;
; Stack Use:	None
;
; Returns:		Nothing
;
;--------------------------------------------------------------------

OS_SetPalette_		proc

	push		ecx
	mov		gunsArray,eax					; Collect passed parameters
	mov		startGun,dx
	mov		endGun,bx

	mov		ecx,ebx							; Calculate the loop count, based
	sub		ecx,edx							;	on the start & end registers
	inc		ecx
	mov		edx,ecx							; Save the result
	shl		ecx,1								; Multiply the result by 3 (for RGB)
	add		ecx,edx

	pushfd										; Turn off interrupts
	cli

	push		esi								; Save SI & DS
	push		ds

	cld											; Clear the direction flag...
	xor		eax,eax							;	... and zero AX & DX
	xor		edx,edx

	mov		esi,gunsArray					; Get the address to the passed array
	mov		dx,PALETTE_PORT				; Get the port number
	mov		ax,startGun						; Get the first color...
	out		dx,al								;	... and set it

	inc		dx									; Next port

SetPaletteStartLoop:

	lodsb											; Get the red, green, or blue value	
	shr		al,2								; Convert 8-bit to 6-bit
	out		dx,al								; Set the value
	loop		SetPaletteStartLoop			; Next register

	pop		ds									; Restore SI & DS
	pop		esi
	popfd											; Turn interrupts back on
	pop		ecx
	ret

OS_SetPalette_		endp



;---[ GetPalette ]---------------------------------------------------
;
; Gets the VGA palette registers
;
; Parameters:	AX = pointer to character array of guns information
;					DX = starting register number
;					BX = ending register number
;
; Stack Use:	None
;
; Returns:		Nothing
;
;--------------------------------------------------------------------

OS_GetPalette_		proc

	push		ecx
	mov		gunsArray,eax					; Collect passed parameters
	mov		startGun,dx
	mov		endGun,bx

	mov		ecx,ebx							; Calculate the loop count, based
	sub		ecx,edx							;	on the start & end registers
	inc		ecx
	mov		edx,ecx							; Save the result
	shl		ecx,1								; Multiply the result by 3 (for RGB)
	add		ecx,edx

	pushfd										; Turn off interrupts
	cli

	push		edi								; Save SI & DS
	push		ds

	cld											; Clear the direction flag...
	xor		eax,eax							;	... and zero AX & DX
	xor		edx,edx

	mov		edi,gunsArray					; Get the address to the passed array
	mov		dx,3C7h							; Get the port number
	mov		ax,startGun						; Get the first color...
	out		dx,al								;	... and set it
	mov		dx,3C9h							; Registers port number?

GetPaletteStartLoop:

	in			al,dx								; Get the register...
	shl		al,2								;	... convert 6-bit to 8-bit
	stosb											;	... and store in gunsArray

	loop		GetPaletteStartLoop			; Next register

	pop		ds									; Restore SI & DS
	pop		edi
	popfd											; Turn interrupts back on
	pop		ecx
	ret

OS_GetPalette_		endp



;---[ OS_GetStackPtr ]---------------------------------------------------
;
; Gets current value of the top of the stack. This function is OS
;	and Memory model dependent
;
; Parameters:	None
; Stack Use:	None
;
; Returns:	The current top of stack
;
;--------------------------------------------------------------------

OS_GetStackPtr_		proc

	mov		eax,esp
	ret

OS_GetStackPtr_		endp


;---[ CopyPixelLine_ASM ]---------------------------------------------------
;
; Copies pixels from one buffer to another
;
; Parameters:	AX = pointer to ASMCP structure
;
; Stack Use:	None
;
; Returns:		Nothing
;
; This function is called from CopyPixelsNormal and CopyPixelsSRLE
; in OSGRAPH.CPP.  The source cel will either be uncompressed or
; SRLE compressed, as indicated by the SRLE flag.
;
; Uncompressed Cel:
;
;	The source buffer pointer is set to the beginning of the cel, and
;	the source buffer offset is the total number of bytes into the cel
;	to the first pixel to copy.
;
; SRLE-Compressed Cel:
;
;	The source buffer pointer is set to the beginning of the scan
;	line to copy and the source buffer offset is the total number
;	of bytes into the line to the first pixel to copy.
;
; HDIR:
;
;	Cels that are to be flipped or mirrored horizontally will be done
;	so by copying the source cel data into the destination buffer in
;	the reverse order of normal cels.  If HDIR is -1, the copy will
;	be done right to left; if 1, the copy will be done normally.
;
;--------------------------------------------------------------------

OS_CopyPixelLine_ASM_		proc

; locals
	acp		equ	-10H	; Our ASMCP instance
	cpEndMark	equ	-0cH	; How we know we're done with a row
	cpSkipBytes 	equ	-8	; How many bytes of leading skip?
	cpIgnored	equ	-4	; How many pixels do we ignore?
; locals

	push		ebx								; Save off registers we're going to use
	push		ecx
	push		edx
	push		esi

	enter		10H,0	; reserve 16 bytes on stack

	mov		esi,eax

CopyPixelsSetCol:

	cmp		[esi].cpHDir,0					; Check the horizontal direction flag
	jl			CopyPixelsSetRtoL				;	-1? We're going backwards
	mov		edx,0								;  else, set the column counter to 0
	mov		eax,[esi].cpBufWide
	mov		dword ptr cpEndMark[ebp],eax					;	... set the end mark ...
	mov		ebx,[esi].cpSrcBuf			; Get the addr. of the source buffer...
	add		ebx,[esi].cpSrcOffset		;	... add the srcOffset
	jmp		CopyPixelsStartLoop			;	... and away we go.

CopyPixelsSetRtoL:

	mov		edx,[esi].cpBufWide			; Set the column counter to the buf wide
	mov		ebx,[esi].cpSrcBuf			; Get the addr. of the source buffer...
	add		ebx,[esi].cpSrcOffset		;	... add the srcOffset...
	add		ebx,[esi].cpBufWide			;	... add the buffer width...
	sub		ebx,1								;	... and adjust to be true rt. edge
	mov		dword ptr cpEndMark[ebp],0						; Set the end mark to 0

CopyPixelsStartLoop:

	mov		ecx,[esi].cpDstBuf			; Get the addr. of the dest. buffer...
	add		ecx,[esi].cpDstOffset		;	... add the dstOffset
	cmp		[esi].cpSRLEFlag,0			; Is this an SRLE cel?
	je			CopyPixelsStartNormal		;  No, just do the normal stuff

CopyPixelsStartSRLE:

	xor		eax,eax
	mov		al,[ebx]							; Get the number of skip bytes...
	mov		byte ptr cpSkipBytes[ebp],al					; store the number of skip bytes
	cmp		[esi].cpSrcOffset,eax		; compare skip bytes to offset
	jg			CopyPixelsNotInSkip			; offset is past the skip
	sub		eax,[esi].cpSrcOffset		; subtract offset from skip bytes
	mov		byte ptr cpSkipBytes[ebp],al
	mov		byte ptr cpIgnored[ebp],0						; don't ignore any pixels
	jmp		CopyPixelsInSkip

CopyPixelsNotInSkip:	  
	mov		eax,[esi].cpSrcOffset
	mov		byte ptr cpIgnored[ebp],al					; ignore the offset...
	mov		al,byte ptr cpSkipBytes[ebp]
	sub		byte ptr cpIgnored[ebp],al					; ... minus the skip bytes
	mov		byte ptr cpSkipBytes[ebp],0
			
CopyPixelsInSkip:

	xor		eax,eax
	add		al,byte ptr cpSkipBytes[ebp]	; Inc dst ptr by # skip bytes
	add		ecx,eax				; Inc dst ptr by # skip bytes
	inc		ebx								; Move the ptr to the run count
	movzx		eax,word ptr [ebx]			; Get the run count
	cmp		eax,0
	je			CopyPixelsExit					; Run count is 0, get out of this line
	cmp		eax,[esi].cpBufWide
	jg			CopyPixelsBigBuffer
	mov		[esi].cpBufWide,eax			; tell the buffer how wide the run count is

CopyPixelsBigBuffer:

	add		ebx,2								; move src data past run count

CopyPixelsStartNormal:

	movzx		eax,[ebx]						; Get the src pixel into AL...
	cmp		[esi].cpDoSkip,0				; Are we ignoring skip?
	je			CopyPixelsCheckForCLUT		;	Yes, just go on
	cmp		al,SKIP_COLOR					;	No, is the pixel skip?
	je			CopyPixelsNextPixel			;	Yes, ignore the pixel

CopyPixelsCheckForCLUT:

	cmp		[esi].cpCLUT,0					; Is there a CLUT?
	je			CopyPixelsMovePixel			;	No, just continue
	cmp		eax,[esi].cpCLUTIdx			; Is the pixel within the CLUT range?
	jl			CopyPixelsMovePixel			;	Pixel value too low for CLUT

CopyPixelsCheckCLUT:

	sub		eax,[esi].cpCLUTIdx			; Subtract the CLUT start from pixel
	cmp		eax,[esi].cpCLUTSize			;	and see if we're still in range
	jng		CopyPixelsDoCLUT				; We're in range, do the substitution
	mov		al,[ebx]							; Pixel value too high for CLUT
	jmp		CopyPixelsMovePixel
	
CopyPixelsDoCLUT:

	push		esi								; Save the address of our structure
	mov		esi,[esi].cpCLUT				; Get the address of our CLUT
	add		esi,eax
	mov		al,[esi]							; Use the pixel value as an index
	pop		esi								; Restore the address of our structure
													; AL now has substituted pixel value

CopyPixelsMovePixel:

	mov		[ecx],al							;	... and move it into the dstBuf
	jmp		CopyPixelsNextPixel

CopyPixelsNextPixel:

	add		edx,[esi].cpHDir				; Next column (or prev column)
	add		ebx,[esi].cpHDir				; Move src ptr to next (prev) src byte
	inc		ecx		  		; Move dst ptr to next dst byte
	cmp		edx,dword ptr cpEndMark[ebp]		; Have we copied an entire row?
	jne		CopyPixelsStartNormal		;	No, do another row

CopyPixelsExit:

	leave

	pop		esi								; Restore registers we saved
	pop		edx
	pop		ecx
	pop		ebx
	ret

OS_CopyPixelLine_ASM_		endp


_OSCODE	ENDS
			END
