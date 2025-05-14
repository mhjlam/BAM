;
;	VESA.ASM
;
;	November 18, 1994  JDH
;	(c) Copyright 1994, Tachyon, Inc.  All rights reserved.
;
;	Assembly routines for VESA functions, including:
;
;		SetBankData
;
;		VESABlit
;		VESASetBank
;
;		ExtensionsOn
;		ExtensionsOff
;		Video7Blit
;		Video7SetBank
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

			screenWide	dw		0			; Width of screen
			screenHigh	dw		0			; Height of screen
			winSize		dw		0			; Window size (VESA)
			winGran		dw		0			; Window granularity (VESA)

			blitBuf		dd		0			; Address of blit buffer
			blitHeight	dw		0			; Height of blit buffer
			blitWidth	dw		0			; Width of blit buffer
			blitX			dw		0			; X position of blit buffer
			blitY			dw		0			; Rectangle's first line Y coordinate
			videoSeg		dw		0			; Video buffer segment

			BankInfo		struc				; Defines info about video banks
				bankNumber		dw		0
				yOffset			dw		0
				transitionX		dw		0
				padTo8Bytes		dw		0
			BankInfo		ends
			bankData		dw		1920 dup (0)	; Table used to hold bank info
														; NOTE! This assumes 480 screenHigh
			bankWidth	dw		0			; Width in this current bank
			bankNoInc	dw		0			; Bank number increment value
			bankNo		dw		0			; Bank number counter

_OSDATA	ENDS


_OSCODE	SEGMENT 	PARA PUBLIC USE32 'CODE'
			ASSUME	DS:DGROUP
			ASSUME  	CS:_OSCODE

			PUBLIC		SetBankData_,

					VESABlit_,
					VESASetBank_,

					ExtensionsOn_,
					ExtensionsOff_,
					Video7Blit_,
					Video7SetBank_


; General equates

VIDEO_BIOS			equ	10h
SC_INQ				equ	6F00h		; Super cond inquire

V_640x480_256		equ	0101h		; VESA 640x480x256 mode
V_SC_INQ				equ	4F00h		; VESA super cond inquire
V_GET_MODE_INFO	equ	4F01h		; VESA get video mode info
V_SET_MODE			equ	4F02h		; VESA set video mode
V_VID_MEM_SELECT	equ	4F05h		; VESA video memory select
V_SET_WIN_POSN		equ	0000h		; VESA set window position

V7_640x480_256		equ	0067h		; Video 7 640x480x256 mode
V7_GET_MODE			equ	6F04h		; Video 7 get video mode
V7_SET_MODE			equ	6F05h		; Video 7 set video mode

EXT_PAGE_SELECT	equ	00F9h
OUTPUT_REG_WRITE	equ	03C2h
EXT_CTRL_ADDR_REG	equ	03C4h
EXT_CTRL_DATA_REG	equ	03C5h
OUTPUT_REG_READ	equ	03CCh
BANK_SELECT			equ	00F6h
BANK_DATA_SIZE		equ	0008h

; Driver values (these equate to those in OSGRAPH.HPP)

DRV_UNKNOWN			equ	00h
DRV_VIDEO7			equ	01h
DRV_GENOA			equ	02h
DRV_TSENG			equ	03h
DRV_TRIDENT			equ	04h
DRV_VESA				equ	05h

;---[ SetBankData ]--------------------------------------------------
;
; Set the bank data table
;
; Parameters:	AX = Maximum screen width (pixels)
;					DX = Maximum screen height (pixels)
;					BX = Window size (VESA only)
;					CX = Window granularity (VESA only)
;
; Stack Use:	None
;
; Returns:		Nothing
;
;--------------------------------------------------------------------

SetBankData_	proc

	mov		screenWide,ax				; Collect the passed parameters
	mov		screenHigh,dx
	mov		winSize,bx
	mov		winGran,cx

	mov		di,0							; Clear our loop counter
	mov		bankNo,0
	xor		edx,edx						; Clear DX (used for line offset)
	lea		esi,bankData				; Get address of bankData table

SetOneBank:

	movzx		ebx,di						; Get the loop count and...
	shl		ebx,3							; 	...multiply by 8 (BankInfo size)
	movzx		eax,bankNo
	mov		esi+ebx+bankNumber,ax	; Set the bank number
	mov		esi+ebx+yOffset,dx		; Set the line offset

	movzx		eax,winSize					; Get winSize...
	shl		eax,10						;	... multiply by 1024...
	movzx		ecx,screenWide
	sub		eax,ecx						;	... subtract screenWide...
	cmp		eax,edx						;	... and compare to lineOffset
	jl			SetBankTransition			; If less than lineOffset, set some
												;	additional VESA info
SetBankNoTransition:						; No, zero out unused fields

	mov		esi+ebx+transitionX,0	; Zero the transitionX
	mov		esi+ebx+padTo8Bytes,0	; Zero the padTo8Bytes
	add		dx,screenWide				; Add screen width to the line offset
	jmp		SetBankContinue

SetBankTransition:

	movzx		eax,winSize					; Get window size...
	shl		eax,10						;	... multiply by 1024...
	sub		eax,edx						;	... subtract line offset...
	mov		esi+ebx+transitionX,ax	;	... and stuff it into transitionX

	movzx		edx,screenWide				; Get screen width into lineOffset...
	sub		edx,eax						;	... and subtract previous result

	push		edx
	xor		edx,edx						; Clear DX for the divide
	movzx		eax,winSize					; Get window size...
	div		winGran						;	... divide by window granularity...
	add		bankNo,ax					;	... and adjust the bank number by it
	pop		edx
	
SetBankContinue:

	inc		di								; Next loop...
	cmp		di,screenHigh				; Are we done yet?
	jne		SetOneBank					; No, continue

	ret

SetBankData_	endp




;=============================================================================
;
; VESA Routines
;
;=============================================================================


;---[ VESABlit ]-----------------------------------------------------
;
; Blit to video, specific to VESA
;
; Parameters:	AX = Address of blit source buffer
;					DX = Blit buffer X position
;					BX = Blit buffer Y position
;					CX = Blit buffer width
;
; Stack Use:	Blit buffer height
;					Bank number increment
;					Video buffer segment
;
; Returns:		unknown
;
;--------------------------------------------------------------------

VESABlit_		proc

	mov		blitBuf,eax						; Collect the passed parameters
	mov		blitX,dx
	mov		blitY,bx
	mov		blitWidth,cx

	push		ebp								; Save registers
	mov		ebp,esp
	push		es

	mov		eax,[ebp+8]						; Retrieve parameters from the stack
	mov		blitHeight,ax
	mov		eax,[ebp+12]
	mov		bankNoInc,ax
	mov		eax,[ebp+16]
	mov		videoSeg,ax
	mov		es,videoSeg

	mov		esi,blitBuf
	movzx		eax,blitY						; Load the starting Y...
	shl		eax,3								; 	... and multiply by 8
													; -> This gives us an offset into the
													;	  bankData table of our starting Y

	lea		ebx,bankData					; Get the address of the bankData table
	add		ebx,eax							;	... and add our offset
	movzx		edx,[ebx]						; Get the bank number from the bankData

	call		VESASetBank_					; Set bank number of first line

VESANextLine:

	movzx		edi,[ebx].yOffset				; Get the line's offset into bank ...
	add		di,blitX							;	... and add the blit buffer X
	movzx		ecx,[ebx].transitionX		; Get the bank transition's X coord
													;	(0 indicates no transition)
	or			ecx,ecx							; Bank transition in this line?
	jnz		VESABankTransitionLine		; Yes

	mov		eax,edi							; Get the line's first pixel's offset into buffer
	and		eax,3								; Is pixel on a double word boundary?
	jz			VESASetDWordWidth				; Yes

	mov		ecx,4								; Get number of pixels per dword...
	sub		ecx,eax							;	... subtract to get the offset
	mov		eax,ecx							;		 to the next dword
	cmp		ax,blitWidth					; Is next dword outside of rectangle?
	jb			VESAAlignOnDWord				; No

	movzx		ecx,blitWidth					; Get rect's width in pixels
	mov		eax,ecx

VESAAlignOnDWord:

	rep		movsb								; Draw pixel by pixel to next dword

VESASetDWordWidth:

	movzx		ecx,blitWidth					; Get rect's width in pixels...
	sub		ecx,eax							;	... calc pixels remaining in width...
	mov		eax,ecx
	shr		ecx,2								;	... and divide by 2 (# dwords)
	rep		movsd								; Draw 4 pixels at a time
	mov		ecx,eax							; Get rect's remaining width prior
													;	to double word draw
	and		ecx,3								; Get rect's remaining width in pixels
	rep		movsb								; Draw remaining pixels pixel by pixel

VESAEndOfLine:

	add		ebx,BANK_DATA_SIZE			; Advance to next line's bank info entry
	dec		edx								; Are there more lines to blit?
	jnz		VESANextLine					; Yes

	jmp		VESABRTSExit					; Blit of rectangle is complete

VESABankTransitionLine:

	mov		bankWidth,0						; Default to line not in current bank
	mov		eax,ecx							; Get X coordinate of transition point
	sub		ax,blitX							; Is part of line in current bank?
	jbe 		VESAAfterBankTransition		; No

	movzx		ecx,blitWidth					; Get rect's width in pixels
	cmp		ecx,eax							; Is line completely in current bank?
	jbe		VESASetWidthInThisBank		; Yes

	mov		ecx,eax							; Get line's width in current bank

VESASetWidthInThisBank:

	mov		bankWidth,cx					; Save line's width in current bank
	mov		eax,edi							; Get line's starting pixel offset
	and		eax,3								; Is pixel on a double word boundary?
	jz			VESASetDWordWidthBT			; Yes

	mov		ecx,4								; Get number of pixels per dword...
	sub		ecx,eax							;	... subtract to get the offset
	mov		eax,ecx							;		 to the next dword
	mov		eax,ecx
	cmp		ax,blitWidth					; Is next dword outside of rectangle?
	jb			VESAAlignOnDWordBT			; No

	movzx		ecx,blitWidth					; Get rect's width in pixels
	mov		eax,ecx

VESAAlignOnDWordBT:

	rep		movsb								; Draw pixel by pixel to next dword

VESASetDWordWidthBT:

	movzx		ecx,bankWidth					; Get rect's width in pixels...
	sub		ecx,eax							;	... calc pixels remaining in width...
	mov		eax,ecx
	shr		ecx,2								;	... and divide by 2 (# dwords)
	rep		movsd								; Draw 4 pixels at a time
	mov		ecx,eax							; Get rect's remaining width prior
													;	to double word draw
	and		ecx,3								; Get rect's remaining width in pixels
	rep		movsb								; Draw remaining pixels pixel by pixel

VESAAfterBankTransition:

	mov		blitHeight,dx					; Save number of lines left to blit

	movzx		edx,[ebx].bankNumber			; Advance current bank number...
	add		dx,bankNoInc					;	... to next bank

	call		VESASetBank_					; Set bank number of next bank

	xor		edi,edi							; Clear pixel offset into bank
	movzx		ecx,blitWidth					; Get rect's width in pixels
	sub		cx,bankWidth					; Is line complete?
	jz			VESAEndOfLine					; Yes

	test		bankWidth,0xFFFF				; Was part of line in previous bank?
	mov		bankWidth,cx					; Set line's width in current bank
	jnz		VESACrossBankBoundary		; Part of line was in previous bank

	movzx		edi,blitX						; Get rect's upper left coordinate...
	sub		di,[ebx].transitionX			;	... subtract transitionX...
													;		 (Result is first X this bank)
	mov		eax,edi
	and		eax,3								; Is pixel on a double word boundary?
	jz			VESASetDWordWidthAT			; Yes

	mov		ecx,4								; Get number of pixels per dword...
	sub		ecx,eax							;	... subtract to get the offset
													;		 to the next dword

VESACrossBankBoundary:

	mov		eax,ecx							; Get offset to next double word
	cmp		ax,blitWidth					; Is next dword outside of rectangle?
	jb			VESAAlignOnDWordAT			; No

	movzx		ecx,blitWidth					; Get rect's width in pixels
	mov		eax,ecx

VESAAlignOnDWordAT:

	rep		movsb		 						; Draw pixel by pixel to next dword

VESASetDWordWidthAT:

	movzx		ecx,bankWidth					; Get rect's width in pixels...
	sub		ecx,eax	  						;	... calc pixels remaining in width...
	mov		eax,ecx
	shr		ecx,2								;	... and divide by 2 (# dwords)
	rep		movsd								; Draw 4 pixels at a time
	mov		ecx,eax							; Get rect's remaining width prior
													;	to dword draw
	and		ecx,3								; Get rect's remaining width in pixels
	rep		movsb								; Draw remaining pixels pixel by pixel

 	add		ebx,BANK_DATA_SIZE			; Advance to next line's bank info entry
	dec		edx								; Are there more lines to blit?
	jnz		VESANextLine					; Yes

VESABRTSExit:

	mov		edx,0								;-DEBUG! Reset bank
	call		VESASetBank_					;/
	pop		es
	pop		ebp
	ret		12		; fix the stack to account for stack passing

VESABlit_		endp



;---[ VESASetBank ]--------------------------------------------------
;
; Set the bank number, specific to VESA
;
; Parameters:	DX = Bank number
;
; Stack Use:	None
;
; Returns:		Nothing
;
;--------------------------------------------------------------------

VESASetBank_	proc

	push		ebx
	mov		eax,V_VID_MEM_SELECT
	mov		ebx,V_SET_WIN_POSN
	int		VIDEO_BIOS
	movzx		edx,blitHeight
	pop		ebx
	ret

VESASetBank_	endp



;=============================================================================
;
; Video7 Routines
;
;=============================================================================


;---[ Video7Blit ]---------------------------------------------------
;
; Blit to video, specific to Video7
;
; Parameters:	unknown
;
; Stack Use:	None
;
; Returns:		unknown
;
;--------------------------------------------------------------------

Video7Blit_		proc

;	mov	esi,pData					; ESI->blit source buffer
;	lea	ebx,bankData				; EBX->video bank info table
;	mov	eax,bufY						; EAX=rectangle's first line Y coordinate
;	shl	eax,3							; EAX=offset to first line's bank info entry
;	add	ebx,eax						; EBX->first line's bank info entry
;
;	movzx	ecx,[ebx].bankNumber		; ECX=bank number	of first line
;
;	V7SetBankNumber()					; Set bank number of first line
;
;	movzx	edx,bufHeight				; EDX=rectangle's height in lines
;
;V7NextLine:
;	movzx	edi,[ebx].yOffset			; EDI=line's offset into bank
;	add	edi,bufX						; EDI=line's first pixel offset into bank
;	movzx	ecx,[ebx].transitionX	; ECX=bank transition's X coordinate (0 indicates no transition)
;	or		ecx,ecx						; Is there a bank transition in this line?
;	jnz	V7BankTransitionLine		; Yes
;
;	mov	eax,edi						; EAX=line's first pixel's offset into buffer
;	and	eax,3							; Is pixel on a double word boundary?
;	jz		V7SetDWordWidth			; Yes
;
;	mov	ecx,4							; ECX=number of pixel's per double word
;	sub	ecx,eax					   ; ECX=offset to next double word
;	mov	eax,ecx						; EAX=offset to next double word
;	cmp	ax,bufWidth					; Is next double word outside of rectangle?
;	jb		V7AlignOnDWord				; No
;
;	movzx	ecx,bufWidth				; ECX=rectangle's width in pixels 
;	mov	eax,ecx						; EAX=rectangle's width	in pixels
;
;V7AlignOnDWord:
;	rep	movsb							; Draw pixel by pixel to next double word
;
;V7SetDWordWidth:
;	movzx	ecx,bufWidth				; ECX=rectangle's width in pixels
;	sub	ecx,eax					  	; ECX=rectangle's remaining width in pixels
;	mov	eax,ecx						; EAX=rectangle's remaining width in pixels
;	shr	ecx,2							; ECX=rectangle's remaining width in double words
;	rep	movsd							; Draw 4 pixels at a time
;	mov	ecx,eax						; ECX=rectangle's remaining width prior to double word draw
;	and	ecx,3							; ECX=rectangle's remaining width in pixels
;	rep	movsb							; Draw remaining pixels pixel by pixel
;
;V7EndOfLine:
;	add	ebx,BANK_DATA_SIZE		; EBX->next line's bank info entry
;	dec	edx							; Are there more lines to blit?
;	jnz	V7NextLine					; Yes
;
;	jmp	V7BRTSExit					; Blit of rectangle is complete
;
;V7BankTransitionLine:
;	mov	widthInThisBank,0			; Default to line not in current bank
;	mov	eax,ecx						; EAX=X coordinate of transition point
;	sub	eax,bufX						; Is part of line in current bank?
;	jbe 	V7AfterBankTransition	; No
;
;	movzx	ecx,bufWidth				; ECX=rectangle's width in pixels
;	cmp	ecx,eax						; Is line completely in current bank?
;	jbe	V7SetWidthInThisBank		; Yes
;
;	mov	ecx,eax						; ECX=line's width in current bank
;
;V7SetWidthInThisBank:
;	mov	widthInThisBank,cx		; Save line's width in current bank
;	mov	eax,edi						; EAX=line's starting pixel offset
;	and	eax,3							; Is pixel on a double word boundary?
;	jz		V7SetDWordWidthBT			; Yes
;
;	mov	ecx,4							; ECX=number of pixel's per double word
;	sub	ecx,eax						; ECX=offset to next double word
;	mov	eax,ecx						; EAX=offset to next double word
;	cmp	ax,bufWidth					; Is next double word outside of rectangle?
;	jb		V7AlignOnDWordBT			; No
;
;	movzx	ecx,bufWidth				; ECX=rectangle's width in pixels
;	mov	eax,ecx						; EAX=rectangle's width	in pixels
;
;V7AlignOnDWordBT:
;	rep	movsb							; Draw pixel by pixel to next double word
;
;V7SetDWordWidthBT:
;	movzx	ecx,widthInThisBank		; ECX=rectangle's width in pixels in current bank
;	sub	ecx,eax						; ECX=rectangle's remaining width in pixels in current bank
;	mov	eax,ecx						; EAX=rectangle's remaining width in pixels in current bank
;	shr	ecx,2							; ECX=rectangle's remaining width in double words in current bank
;	rep	movsd							; Draw 4 pixel's at a time
;	mov	ecx,eax						; ECX=rectangle's remaining width in current bank prior to double word draw
;	and	ecx,3							; ECX=rectangle's remaining width in pixels in current bank
;	rep	movsb							; Draw remaining pixels in current bank pixel by pixel
;
;V7AfterBankTransition:
;	mov	bufHeight,dx				; Save number of lines left to blit
;
;	movzx	ecx,[ebx].bankNumber		; ECX=current bank number
;	inc	ecx							; ECX=next bank number
;
;	V7SetBankNumber()					; Set bank number of next bank
;
;	movzx	edx,bufHeight				; EDX=number of lines left to blit
;	xor	edi,edi						; EDI=new bank's first pixel's offset
;	movzx	ecx,bufWidth				; ECX=rectangle's width in pixels
;	sub	cx,widthInThisBank		; Is line complete?
;	jz		V7EndOfLine					; Yes
;
;	test	widthInThisBank,0xFFFF	; Check to see if part of line was in previous bank?
;	mov	widthInThisBank,cx		; Set line's width in current bank
;	jnz	V7CrossBankBoundary		; Part of line was in previous bank
;
;	mov	edi,bufX						; EDI=rectangle's upper left X coordinate
;	sub	di,[ebx].transitionX		; EDI=rectangle's first pixel offset in current bank
;	mov	eax,edi						; EAX=rectangle's first pixel offset in current bank
;	and	eax,3							; Is pixel on a double word boundary?
;	jz		V7SetDWordWidthAT			; Yes
;
;	mov	ecx,4							; ECX=number of pixel's per double word
;	sub	ecx,eax						; ECX=offset to next double word
;
;V7CrossBankBoundary:
;	mov	eax,ecx						; EAX=offset to next double word
;	cmp	ax,bufWidth					; Is next double word outside of rectangle?
;	jb		V7AlignOnDWordAT			; No
;
;	movzx	ecx,bufWidth				; ECX=rectangle's width in pixels
;	mov	eax,ecx						; EAX=rectangle's width	in pixels
;
;V7AlignOnDWordAT:
;	rep	movsb							; Draw pixel by pixel to next double word
;
;V7SetDWordWidthAT:
;	movzx	ecx,widthInThisBank		; ECX=rectangle's width in pixels in current bank
;	sub	ecx,eax						; ECX=rectangle's remaining width in pixels
;	mov	eax,ecx						; EAX=rectangle's remaining width in pixels
;	shr	ecx,2							; ECX=rectangle's remaining width in double words
;	rep	movsd							; Draw 4 pixels at a time
;	mov	ecx,eax						; ECX=rectangle's remaining width prior to double word draw
;	and	ecx,3							; ECX=rectangle's remaining width in pixels
;	rep	movsb							; Draw remaining pixels pixel by pixel
;	add	ebx,BANK_DATA_SIZE		; EBX->next line's bank info entry
;	dec	edx							; Are there more lines to blit?
;	jnz	V7NextLine					; Yes
;
;V7BRTSExit:
	ret

Video7Blit_		endp



;---[ Video7SetBank ]------------------------------------------------
;
; Set the bank number, specific to Video7
;
; Parameters:	unknown
;
; Stack Use:	None
;
; Returns:		unknown
;
;--------------------------------------------------------------------

Video7SetBank_	proc

;	call	ExtensionsOn
;	mov	ah,cl
;	and	ah,001h
;	mov	al,EXT_PAGE_SELECT
;	mov	dx,EXT_CTRL_ADDR_REG
;	out	dx,ax
;	mov	dx,OUTPUT_REG_READ
;	in		al,dx
;	mov	ah,cl
;	and	ah,002h
;	shl	ah,4
;	and	al,0DFh
;	or		al,ah
;	mov	dx,OUTPUT_REG_WRITE
;	out	dx,al
;	mov	al,BANK_SELECT
;	mov	dx,EXT_CTRL_ADDR_REG
;	out	dx,al
;	inc	dx
;	in		al,dx
;	and	al,0F0h
;	and	cl,00Ch
;	or		al,cl
;	shr	cl,2
;	or		al,cl
;	out	dx,al
;	call	ExtensionsOff
	ret

Video7SetBank_	endp


;---[ ExtensionsOn ]-------------------------------------------------
;
; Turn Extensions on
;
; Parameters:	None
;
; Stack Use:	None
;
; Returns:		Nothing
;
;--------------------------------------------------------------------

ExtensionsOn_	proc

	mov	ax,00AE06h
	mov	dx,EXT_CTRL_ADDR_REG
	out	dx,ax

	ret

ExtensionsOn_	endp



;---[ ExtensionsOff ]------------------------------------------------
;
; Turn Extensions off
;
; Parameters:	None
; Returns:		Nothing
;
;--------------------------------------------------------------------

ExtensionsOff_	proc

	mov	ax,00EA06h
	mov	dx,EXT_CTRL_ADDR_REG
	out	dx,ax

	ret

ExtensionsOff_	endp

_OSCODE	ENDS
			END




