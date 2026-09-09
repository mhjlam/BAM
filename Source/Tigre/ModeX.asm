;
;	XMODE.ASM
;
;	October 18, 1994  JDH
;	(c) Copyright 1993, Tachyon, Inc.  All rights reserved.
;
;	Assembly routines for XMODE functions, including:
;
;		SetXMode
;		SetXModeWritePlane
;		SetXModeReadPlane
;		XModeFlipPage
;		FillScreen
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
.486
DGROUP	GROUP		_OSCODE

_OSDATA	SEGMENT	PARA PUBLIC USE32 'DATA'

	_pVGAMem		dd	0	; current Draw address
	_startAddrHiRegIndex	dw	0	; start address high reg index
	_curDrawPage		dw	0	; current Page to draw into
	useByteBlit		db	0	; TRUE (1) when word & double word blits aren't supported

	PUBLIC		_pVGAMem,
			_curDrawPage

_OSDATA	ENDS

_OSCODE	SEGMENT 	PARA PUBLIC USE32 'CODE'
			ASSUME	DS:DGROUP
			ASSUME  	CS:_OSCODE

			PUBLIC			SetXMode_,
						XModeFlipPage_,
						FillScreen_,
						ABlit_,
						AFBlit_,
						ARBlit_,
						SyncWithVBL_


; General equates

; mode X stuff, used in setting mode X
SC_INDEX		equ	03c4h	; sequence controller index register
GC_INDEX		equ	03ceh	; graphics controller index register
CRTC_INDEX		equ	03d4h	; CRT controller index register
MAP_MASK		equ	2	; map mask register index in SC
MEMORY_MODE		equ	4	; memory mode register index in SC
START_ADDRESS_HIGH	equ	0ch	; start address high reg index in CRTC
GRAPHICS_MODE		equ	5
MISCELLANEOUS		equ	6
MAX_SCAN_LINE		equ	9
UNDERLINE		equ	14h
MODE_CONTROL		equ	17h
READ_MAP		equ	4	; read map register index in GC
SCREEN_WIDTH		equ	320
SCREEN_HEIGHT		equ	400
VGA_MEMORY		equ	0a0000h
X_MODE_PAGE_MASK	equ	008000h
VIDEO_BUFFER_WIDTH	equ	80


;---[ SetXMode ]-----------------------------------------------------
;
; Set 320 x 400 Mode
;
; Parameters:	None
; Returns:		Nothing
;
;--------------------------------------------------------------------

IMAGE_BUFFER_WIDTH	equ	15
IMAGE_BUFFER_HEIGHT	equ	29
IMAGE_BUFFER_SIZE	equ	IMAGE_BUFFER_WIDTH*IMAGE_BUFFER_HEIGHT
STACK_FRAME_SIZE	equ	((IMAGE_BUFFER_SIZE+7)/4)*4
X_COORDINATE		equ	0
Y_COORDINATE		equ	0
imageBuffer		equ	[ebp-STACK_FRAME_SIZE+4]
numberOfBlits		equ	[ebp-STACK_FRAME_SIZE]

; set mode X - called by SetVGA()
SetXMode_	proc

	push	eax				;Save EAX on stack
	push	ebx				;Save EBX on stack
	push	ecx				;Save ECX on stack
	push	edx				;Save EDX on stack
	push	edi				;Save EDI on stack
	push	ebp				;Save EBP on stack
	mov	ebp,esp				;EBP->bottom of stack frame
	sub	esp,STACK_FRAME_SIZE		;ESP->top of stack frame
	; First, go to normal 320x200 256-color mode, which is really a
	; 320x400 256-color mode with each line scanned twice.

	mov	ax,0x13				; AH = 0 means mode set, AL = 0x13 selects
						; 256-color graphics mode
	int	10h				; BIOS video interrupt

	; Change CPU addressing of video memory to linear (not odd/even,
	; chain, or chain 4), to allow us to access all 256K of display
	; memory.  When this is done, VGA memory will look just like memory
	; in modes 10h and 12h, except that each byte of display memory will
	; control one 256-color pixel, with 4 adjacent pixels at any given
	; address, one pixel per plane.

	mov 	dx, SC_INDEX
	mov	al, MEMORY_MODE
	out 	dx,al
	inc	dx
	in	al,dx
	and	al,0xf7				; turn off chain 4 mode
	or	al,04h				; turn off odd/even
	out	dx,al

	mov	dx,GC_INDEX
	mov	al,GRAPHICS_MODE
	out 	dx,al
	inc	dx
	in	al,dx
	and	al,0xef				; turn off odd/even
	out	dx,al
	dec	dx
	mov	al,MISCELLANEOUS
	out	dx,al
	inc	dx
	in		al,dx
	and	al,0xfd				; turn off chain

	; Now clear the whole screen, since the mode 13h set only
	; cleared 64K out of the 256K of display memory.  Do this before
	; we switch the CRTC out of mode 13h, so we don't see garbage
	; on the screen when we make the switch.

	mov	dx, SC_INDEX
	mov	ax, 0fh	    			; write to all 4 planes
	shl	ax, 8
	add	ax, MAP_MASK
	out	dx,ax

	mov	edi,VGA_MEMORY
	xor	eax,eax			
	mov	ecx,0x4000			; # of dwords in 64K 
	cld
	rep	stosd				; clear all of display memory

	; Tweak the mode to 320x400 256-color more by not scanning each
	; line twice.

	mov	dx,CRTC_INDEX
	mov	al,MAX_SCAN_LINE
	out	dx,al
	inc	dx
	in	al,dx
	and	al,0xe0				; set maximum scan line = 0
	out	dx,al
	dec	dx

	; Change CRTC scanning from doubleword mode to byte mode, allowing
	; the CRTC to scan more than 64K of video data.

	mov	al,UNDERLINE
	out	dx,al
	inc	dx
	in	al,dx
	and	al,0xbf		; turn off double word
	out	dx,al
	dec	dx
	mov	al,MODE_CONTROL
	out	dx,al
	inc	dx
	in	al,dx
	or	al,40h				; turn on the byte mode bit, so memory is
						; scanned for video data in a purely
						; linear way, just as in modes 10h and 12h
	out	dx,al

	; setup the page addresses

	mov	_pVGAMem, VGA_MEMORY + X_MODE_PAGE_MASK	;Set video buffer to page 1
	mov	ebx,IMAGE_BUFFER_HEIGHT		;EBX=number of lines in image
	lea	edi,imageBuffer			;EDI->first byte of image buffer

SXMBuildImage:
	mov	ecx,4				;ECX=number of leading zeros in each image line
	xor	al,al				;AL=zero
	rep	stosb				;Put leading zeros into image line
	mov	eax,31323334h			;EAX=color pattern
	stosd					;Put color pattern into image line
	mov	ecx,7				;ECX=number of trailng zeros in each image line
	xor	al,al				;AL=zero
	rep	stosb				;Put trailing zeros into image lin
	dec	ebx				;EBX=number of image lines remaining to be built
	jnz	SXMBuildImage			;Loop until all image lines have been built

	xor	eax,eax				;EAX=0
	push	eax				;Add dummy argument to stack
	mov	eax,IMAGE_BUFFER_WIDTH		;EAX=number of bytes in buffer line (span)
	push	eax				;Add image height to stack
	mov	eax,IMAGE_BUFFER_HEIGHT		;EAX=number of lines in image
	push	eax				;Add image height to stack
	mov	eax,IMAGE_BUFFER_WIDTH		;EAX=number of bytes in each image line
	push	eax				;Add image width to stack
	mov	ebx,X_COORDINATE		;EBX=image X coordinate
	mov	ecx,Y_COORDINATE		;ECX=image Y coordinate
	lea	edx,imageBuffer			;EDX->first byte of image buffer
	call	ABlit_				;Blit image buffer to video buffer 

	mov	DWORD PTR numberOfBlits,15	;Set number of times to reverse blit image

SXMRBlit:
	push	eax				;Add dummy argument to stack
	mov	eax,IMAGE_BUFFER_HEIGHT		;EAX=number of lines in image
	push	eax				;Add image height to stack
	mov	eax,IMAGE_BUFFER_WIDTH		;EAX=number of bytes in each image line
	push	eax				;Add image width to stack
	mov	ebx,X_COORDINATE		;EBX=image X coordinate
	mov	ecx,Y_COORDINATE		;ECX=image Y coordinate
	lea	edx,imageBuffer			;EDX->first byte of image buffer
	call	ARBlit_				;Blit video buffer data to image buffer

	mov	ebx,IMAGE_BUFFER_HEIGHT		;EBX=number of lines in image
	xor	eax,eax				;EAX=0
	lea	edi,imageBuffer			;EDI->first byte of image buffer

SXMNextLine:
	mov	ecx,4				;ECX=number of leading zeros in each image line

SXMFirst4Bytes:					;Or image byte into AL
	or	al,[edi]			;EDI->next byte of image buffer
	lea	edi,[edi+1]			;Loop until all leading zeros have been processed
	loop	SXMFirst4Bytes

	lea	edi,[edi+4]			;EDI->first trailing zero of image line
	mov	ecx,7				;ECX=number of trailing zeros in each image line

SXMLast7Bytes:
	or	al,[edi]			;Or image byte into AL
	lea	edi,[edi+1]			;EDI->next byte of image buffer
	loop	SXMLast7Bytes			;Loop until all trailing zeros have been processed
	jnz	SXMSetUseByteBlit		;One or more of the zero bytes is corrupt

	dec	ebx				;Update number of plane lines remaining to test
	jnz	SXMNextLine			;Loop until all plane lines have been tested

	dec	DWORD PTR numberOfBlits		;Update number of blits remaining to be done
	jnz	SXMRBlit			;Loop until all blits have been done
	jmp	SXMExit				;All blits have been done and no corrupt zero bytes found

SXMSetUseByteBlit:
	mov	BYTE PTR useByteBlit,1		;Set flag to force byte by byte reverse blits

SXMExit:
	; set the current page
	mov	_pVGAMem,VGA_MEMORY 
	mov	_curDrawPage, 0
	mov	_startAddrHiRegIndex,START_ADDRESS_HIGH

	mov	esp,ebp				;Restore stack pointer
	pop	ebp				;Restore EBP from stack
	pop	edi				;Restore EDI from stack
	pop	edx				;Restore EDX from stack
	pop	ecx				;Restore ECX from stack
	pop	ebx				;Restore EBX from stack
	pop	eax				;Restore EAX from stack

	ret					;Return to caller

SetXMode_	endp


;---[ XModeFlipPage ]-----------------------------------------
;
; Flip the XMode page
;
; Parameters:	None
; Returns:	Nothing
;
;--------------------------------------------------------------------

; set mode X write page
XModeFlipPage_	proc

	push	eax
	push	edx

	xor	_curDrawPage,1		; toggle the page bit
	xor	_pVGAMem,X_MODE_PAGE_MASK
	xor	_startAddrHiRegIndex,X_MODE_PAGE_MASK
	mov	dx,CRTC_INDEX
	mov	ax,_startAddrHiRegIndex
	out	dx,ax

	pop	edx
	pop	eax

	ret

XModeFlipPage_	endp

;---[ FillScreen ]-----------------------------------------
;
; Fill the entire XMode memory to one pen number
;
; Parameters:	EAX = Pen Number
; Returns:	Nothing
;
;--------------------------------------------------------------------

; fill mode X memory
FillScreen_	proc
	
	push	eax
	push	ebx
	push	ecx
	push	edx
	push	edi

	mov	ebx,eax		;save the pen number
	mov	dx, SC_INDEX
	mov	ax, 0fh	    	;write to all 4 planes
	shl	ax, 8
	add	ax, MAP_MASK
	out	dx,ax

	mov	edi,_pVGAMem
	mov	eax,ebx		;get the pen number
	mov	ecx,8000	;# of dwords in each plane 
	cld
	rep	stosd		;clear all of display memory

	pop	edi
	pop	edx
	pop	ecx
	pop	ebx
	pop	eax
	ret

FillScreen_	endp

;---[ ABlit ]-----------------------------------------
;
; Blit image buffer data to 320 x 400 video buffer
; Parameters:	EAX = driver (not currently used)
;		EBX = X coordinate
;		ECX = Y coordinate
;		EDX = pointer to image data
;		stack+10h = image width
;		stack+14h = image height
;		stack+18h = image buffer span
;		stack+1Ch = blit type
; Returns:	Nothing
;
;--------------------------------------------------------------------

STACK_ARG_BYTES 	equ	16
ARB_STACK_ARG_BYTES 	equ	12
MASK_AND_COUNT		equ	2
STK_FRAME_SIZE		equ	48h
SHUTTER_BLIT		equ	0FFFFFFFFh
WIPE_BLIT		equ	0FFFFFFFEh
WIPE_THRESHOLD		equ	14400
LEADING_BYTE		equ	08h
LEADING_WORD		equ	04h
TRAILING_WORD		equ	02h
TRAILING_BYTE		equ	01h

bufferWidth		equ	[ebp+10h]
bufferHeight		equ	[ebp+14h]
bufferSpan		equ	[ebp+18h]
ARBlitType		equ	[ebp+18h]
blitType		equ	[ebp+1Ch]

planeBitMask		equ	[ebp-04h]
planeNumber		equ	[ebp-04h]
pVideoBuffer		equ	[ebp-08h]
pVideoData		equ	[ebp-0Ch]
videoLineWrapFactor	equ	[ebp-0Ch]
pImageBuffer		equ	[ebp-10h]
pImageData		equ	[ebp-14h]
imageLineWrapFactor	equ	[ebp-14h]
planeWidth		equ	[ebp-18h]
bytesToBlit		equ	[ebp-1Ch]
lBLWTWTBMask		equ	[ebp-20h]
doubleWordCnt		equ	[ebp-24h]
lTMasksDWCnts		equ	[ebp-44h]
pLTMaskDWCnt		equ	[ebp-48h]
firstLocalVar		equ	[ebp-STK_FRAME_SIZE]

ABlit_		proc
	push	esi				;Save ESI on stack
	push	edi				;Save EDI on stack
	push	ebp				;Save EBP on stack
	mov	ebp,esp				;EBP->bottom of stack frame
	sub	esp,STK_FRAME_SIZE		;ESP->top of stack frame
	mov	esi,ecx				;ESI=Y coordinate
	mov	ecx,STK_FRAME_SIZE/4 		;ECX=number of double words in stack frame
	xor	eax,eax				;EAX=0
	lea	edi,firstLocalVar 		;EDI->first local variable in stack frame
	cld					;Clear direction flag
	rep	stosd				;Set all local variables to 0
	mov	edi,esi				;EDI=Y coordinate
	shl	edi,6				;EDI=Y coordinate * 64
	shl	esi,4				;ESI=Y coordinate * 16
	add	edi,esi				;EDI=Y coordinate * 80 (line's offset into video buffer)
	add	edi,_pVGAMem			;EDI->start of line in video buffer
	mov	esi,ebx				;ESI=X coordinate
	shr	esi,2				;ESI=X coordinate / 4 (byte's offset into video buffer line)
	add	edi,esi				;EDI->top-left byte of destination rectangle in video buffer
	mov	esi,edx				;ESI->start of image rectangle
	mov	eax,bufferWidth			;EAX=width of image rectangle
	mov	planeWidth,eax			;Save plane width of image rectangle
	mul	DWORD PTR bufferHeight 		;EAX=total number of bytes to blit
	add	bytesToBlit,eax			;bytesToBlit=total number of bytes to blit
	jz	ABExit				;bytesToBlit is 0 - nothing to do

	mov	ecx,ebx				;ECX=X coordinate
 	and	ecx,3				;ECX=X coordinate mod 4 (starting plane's plane number)
	mov	al,1				;AL=plane bit mask seed value
	shl	al,cl				;AL=plane bis mask actual value
	mov	ah,al				;AH=plane bit mask actual value
	shl	ah,4				;AH=wrap around to plane 0 flag
	or	ah,al				;AH=plane bit mask actual value and wrap around flag
	cmp	DWORD PTR blitType,SHUTTER_BLIT ;Check for an explicit shutter blit call
	je	ASBPlane			;Do a shutter blit

	cmp	DWORD PTR blitType,WIPE_BLIT 	;Check for an explicit wipe blit call
	je	AWipeBlit			;Do a wipe blit

	cmp	DWORD PTR bytesToBlit,WIPE_THRESHOLD ;Check for a blit that exceeds the shutter blit threshold
	jae	AWipeBlit			;Do a wipe blit

ASBPlane:
	mov	planeBitMask,ah 		;Save this plane's bit mask actual value and wrap around flag
	mov	pImageBuffer,esi 		;Save pointer to first image byte for current plane's line
	mov	pVideoBuffer,edi 		;Save pointer to start of current line in destination rectangle's plane of video buffer
	mov	al,MAP_MASK			;AL=plane map mask register index
	mov	dx,SC_INDEX			;DX=sequence controller index register selector
	out	dx,ax				;Setup for writing to the proper plane for these bytes
	mov	ecx,planeWidth 			;ECX=plane width of image rectangle 
	add	ecx,3				;ECX=extend width to a full 4 plane set
	shr	ecx,2				;ECX=number of bytes in each line of this plane
	mov	eax,bufferHeight 		;EAX=height of destination rectangle
	mul	ecx				;EAX=number of bytes to move in this plane
	sub	bytesToBlit,eax			;Update number of bytes remaining to move after this plane
	mov	edx,ecx				;EDX=number of bytes in each line of this plane
	mov	ecx,edi				;ECX=address of first byte in this line
	and	ecx,1				;ECX=number of bytes to word boundary
	xor	al,al				;AL=0
	sub	al,cl				;AL=FFh or 00h
	and	al,LEADING_BYTE			;AL bit 3=leading byte flag
	sub	edx,ecx				;EDX=number of bytes remaining in line
	jz	ASBSetLTBMask			;No bytes remaining to move

	add	ecx,edi				;ECX=address of first word in this line
	and	ecx,2				;ECX=number of bytes to double word boundary
	cmp	edx,ecx				;EDX=number of bytes remaining in line
	jl	ASBSetTrailingFlags		;No double words or trailing words left to move

	xor	ah,ah				;AH=0
	sub	ah,cl				;AH=FEh or 00h
	and	ah,LEADING_WORD			;AH bit 2=leading word flag
	or	al,ah				;AL bit 2=leading word flag
	sub	edx,ecx				;EDX=number of bytes remaining in line
	jz	ASBSetLTBMask			;No bytes remaining to move

	mov	ecx,edx 			;ECX=number of bytes remaining in line
	shr	ecx,2				;ECX=number of double words in line
	mov	doubleWordCnt,ecx 		;Save double word count
	shl	ecx,2				;ECX=double word count converted to bytes
	sub	edx,ecx				;EDX=number of bytes remaining in line

ASBSetTrailingFlags:
	mov	ecx,edx				;ECX=number of bytes remaining after double words
	and	ecx,TRAILING_WORD		;ECX=number of trailing words
	or	al,cl				;AL bit 1=trailing word flag
	and	edx,TRAILING_BYTE		;EDX=number of bytes remaining in line
	or	al,dl				;AL bit 0=trailing byte flag:bit 1=trailing word flag:bit 2=leading word flag:bit 3=leading byte flag

ASBSetLTBMask:
	mov	dl,al				;DL bit 0=trailing byte flag:bit 1=trailing word flag:bit 2=leading word flag:bit 3=leading byte flag8
	shl	al,4				;AL bit 4=trailing byte flag:bit 5=trailing word flag:bit 6=leading word flag:bit 7=leading byte flag
	or	dl,al				;DL bit 0=trailing byte flag:bit 1=trailing word flag:bit 2=leading word flag:bit 3=leading byte flag:bit 4=trailing byte flag:bit 5=trailing word flag:bit 6=leading word flag:bit 7=leading byte flag
	mov	ebx,bufferHeight 		;EBX=height of destination rectangle

ASBLine:
	mov	pImageData,esi			;Save pointer to first image byte for current line
	mov	pVideoData,edi			;Save pointer to start of this line in video buffer
	rol	dl,1				;Carry=leading byte flag; DL bit 0=leading byte flag:bit 1=trailing byte flag:bit 2=trailing word flag:bit 3=leading word flag:bit 4=leading byte flag:bit 5=trailing byte flag:bit 6=trailing word flag:bit 7=leading word flag
	jnc	ASBLeadingWord			;No carry - no leading byte

	mov	al,[esi]			;AL=byte from image buffer line
	lea	esi,[esi+4]			;ESI->next byte in image buffer for current plane's line of video buffer
	mov	[edi],al			;Move byte from image buffer to video buffer
	lea	edi,[edi+1]			;EDI->next word in video buffer for current plane's line

ASBLeadingWord:
	rol	dl,1				;Carry=leading word flag; DL bit 0=leading word flag:bit 1=leading byte flag:bit 2=trailing byte flag:bit 3=trailing word flag:bit 4=leading word flag:bit 5=leading byte flag:bit 6=trailing byte flag:bit 7=trailing word flag
	jnc	ASBDoubleWords			;No carry - no leading word

	mov	ah,[esi+4]			;AH=byte from image buffer
	lea	esi,[esi+8]			;ESI->next byte in image buffer for current plane's line of video buffer
	mov	al,[esi-8]			;AL=byte from image buffer
	lea	edi,[edi+2]			;EDI->next double word in video buffer for current plane's line
	mov	[edi-2],ax			;Move word from image buffer to video buffer

ASBDoubleWords:
	mov	ecx,doubleWordCnt 		;ECX=number of double words in current plane's line
	jecxz	ASBTrailingWord			;ECX is 0 - there are no double words

ASBNextDoubleWord:
	mov	ah,[esi+8]			;AH=byte from image buffer
	dec	ecx				;ECX=number of double words remaining after this pair
	mov	al,[esi+12]			;AL=byte from image buffer
	bswap	eax				;EAL=AH; EAH=AL
	lea	esi,[esi+16]			;ESI->next byte in image buffer for current plane's line of video buffer
	mov	ah,[esi-12]			;AH=byte from image buffer
	lea	edi,[edi+4]			;EDI->next double word in video buffer
	mov	al,[esi-16]			;AL=byte from image buffer
	mov	[edi-4],eax			;Move double word image data to video buffer
	jnz	ASBNextDoubleWord 		;Loop until all double words have been moved

ASBTrailingWord:
	rol	dl,1				;Carry=trailing word flag; DL bit 0=trailing word flag:bit 1=leading word flag bit 2=leading byte flag:bit 3=trailing byte flag:bit 4=trailing word flag:bit 5=leading word flag:bit 6=leading byte flag:bit 7=trailing byte flag
	jnc	ASBTrailingByte			;No carry - no trailing word

	mov	ah,[esi+4]			;AH=byte from image buffer
	lea	esi,[esi+8]			;ESI->next byte in image buffer for current plane's line of video buffer
	mov	al,[esi-8]			;AL=byte from image buffer
	lea	edi,[edi+2]			;EDI->next byte in video buffer for current plane's line
	mov	[edi-2],ax			;Move word from image buffer to video buffer

ASBTrailingByte:
	rol	dl,1				;Carry=trailing byte flag; DL bit 0=trailing byte flag:bit 1=trailing word flag:bit 2=leading word flag bit 3=leading byte flag:bit 4=trailing byte flag:bit 5=trailing word flag:bit 6=leading word flag:bit 7=leading byte flag
	jnc	ASBFinishedChk			;No carry - no trailing byte

	mov	al,[esi]			;AL=byte from image buffer
	mov	[edi],al			;Move byte from image buffer to video buffer

ASBFinishedChk:	
	dec	ebx				;Update number of plane lines remaining to blit
	jz	ASBNextPlane			;Plane blit is complete when EBX is 0

	mov	esi,pImageData			;ESI->first image byte for previous line
	mov	edi,pVideoData			;EDI->start of previous line in destination rectangle of video buffer
	add	esi,bufferSpan			;ESI->first image byte for this line
	lea	edi,[edi+VIDEO_BUFFER_WIDTH] 	;EDI->start of this line in destination rectangle of video buffer
	jmp	ASBLine				;Move this image line's bytes into video buffer

ASBNextPlane:
	mov	ecx,bytesToBlit			;Check number of bytes to blit count
	jecxz	ABExit				;Blit is complete when bytesToBlit is 0

	mov	DWORD PTR doubleWordCnt,0	;Clear double word count
	dec	DWORD PTR planeWidth 		;Update plane width of plane rectangle
	mov	esi,pImageBuffer 		;ESI->first image byte for previous plane's line
	mov	edi,pVideoBuffer		;EDI->first video byte for this plane
	mov	ah,planeBitMask			;AH=previous plane's bit mask actual value and wrap around flag
	lea	esi,[esi+1]			;ESI->first image byte for this plane's line 
	rol	ah,1				;AH=this plane's bit mask actual value and wrap around flag
	jnc	ASBPlane 			;Blit plane if plane number didn't wrap to 0

	inc	edi		 		;Set video buffer offset to reflect plane 0 wrap around
	jmp	ASBPlane 			;Blit plane 0

ABExit:
	mov	esp,ebp				;Restore stack pointer
	pop	ebp				;Restore EBP from stack
	pop	edi				;Restore EDI from stack
	pop	esi				;Restore ESI from stack

	ret	STACK_ARG_BYTES			;Return to caller

AWipeBlit:
	mov	planeBitMask,ah 		;Save starting plane's bit mask actual value and wrap around flag
	mov	pImageBuffer,esi 		;Save pointer to first image byte for current plane's line
	mov	bl,ah				;BL=starting plane's bit mask actual value and wrap around flag
	mov	pVideoBuffer,edi 		;Save pointer to start of current line in destination rectangle's plane of video buffer
	xor	bh,bh				;Clear ending plane's bit mask
	mov	pVideoData,edi			;Save pointer to start of video buffer rectangle
	lea	edi,lTMasksDWCnts		;EDI->starting plane's leading-double-trailing counts

AWBSetLTMaskDWCnt:
	mov	ecx,planeWidth 			;ECX=plane width of image rectangle 
	add	ecx,3				;ECX=extend width to a full 4 plane set
	shr	ecx,2				;ECX=number of bytes in each line of this plane
	mov	edx,ecx				;EDX=number of bytes in each line of this plane
	mov	ecx,pVideoData			;ECX=address of first byte in this line
	and	ecx,1				;ECX=number of bytes to word boundary
	xor	al,al				;AL=0
	sub	al,cl				;AL=FFh or 00h
	and	al,LEADING_BYTE			;AL bit 3=leading byte flag
	sub	edx,ecx				;EDX=number of bytes remaining in line
	jz	AWBSetLTBMask			;No bytes remaining to move

	add	ecx,pVideoData			;ECX=address of first word in this line
	and	ecx,2				;ECX=number of bytes to double word boundary
	cmp	edx,ecx				;EDX=number of bytes remaining in line
	jl	AWBSetTrailingFlags		;No double words or trailing words left to move

	xor	ah,ah				;AH=0
	sub	ah,cl				;AH=FEh or 00h
	and	ah,LEADING_WORD			;AH bit 2=leading word flag
	or	al,ah				;AL bit 2=leading word flag
	sub	edx,ecx				;EDX=number of bytes remaining in line
	jz	AWBSetLTBMask			;No bytes remaining to move

	mov	ecx,edx 			;ECX=number of bytes remaining in line
	shr	ecx,2				;ECX=number of double words in line
	mov	doubleWordCnt,ecx 		;Save double word count
	shl	ecx,2				;ECX=double word count converted to bytes
	sub	edx,ecx				;EDX=number of bytes remaining in line

AWBSetTrailingFlags:
	mov	ecx,edx				;ECX=number of bytes remaining after double words
	and	ecx,TRAILING_WORD		;ECX=number of trailing words
	or	al,cl				;AL bit 1=trailing word flag
	and	edx,TRAILING_BYTE		;EDX=number of bytes remaining in line
	or	al,dl				;AL bit 0=trailing byte flag:bit 1=trailing word flag:bit 2=leading word flag:bit 3=leading byte flag

AWBSetLTBMask:
	mov	dl,al				;DL bit 0=trailing byte flag:bit 1=trailing word flag:bit 2=leading word flag:bit 3=leading byte flag
	shl	al,4				;AL bit 4=trailing byte flag:bit 5=trailing word flag:bit 6=leading word flag:bit 7=leading byte flag
	or	dl,al				;DL bit 0=trailing byte flag:bit 1=trailing word flag:bit 2=leading word flag:bit 3=leading byte flag:bit 4=trailing byte flag:bit 5=trailing word flag:bit 6=leading word flag:bit 7=leading byte flag
	mov	lBLWTWTBMask,edx		;Save this plane's leading and trailing flags
	mov	ecx,MASK_AND_COUNT 		;ECX=number of variables for each plane
	lea	esi,doubleWordCnt 		;ESI->first common counter
	rep	movsd				;Save the mask and count for this plane
	mov	ah,planeBitMask			;AH=starting plane's bit mask actual value and wrap around flag
	mov	DWORD PTR doubleWordCnt,0	;Clear double word count
	dec	DWORD PTR planeWidth 		;Update plane width of plane rectangle
	jnz	AWBWrapToPlane0Chk		;At least one more plane has data

	rol	bl,1				;BL=next plane's bit mask actual value and wrap around flag
	mov	bh,bl				;BH=last plane's next plane (for an image which spans less than 4 planes)
	mov	bl,ah				;BL=starting plane's bit mask actual value and wrap around flag
	jmp	AWBStart			;Start wipe blit

AWBWrapToPlane0Chk:
	rol	bl,1				;BL=this plane's bit mask actual value and wrap around flag
	jnc	AWBStartChk			;Check for wipe start if plane number didn't wrap to 0

	inc	DWORD PTR pVideoData 		;Set video buffer offset to reflect plane 0 wrap around

AWBStartChk:
	cmp	ah,bl				;All planes' counts have been saved when AH==BL
	jne	AWBSetLTMaskDWCnt		;Loop until all planes' mask and count have been saved

AWBStart:
	mov	esi,pImageBuffer		;ESI->start of image buffer rectangle
	mov	edi,pVideoBuffer		;EDI->start of video buffer rectangle
	mov	pImageData,esi			;Save pointer to start of image buffer rectangle
	mov	pVideoData,edi			;Save pointer to start of video buffer rectangle
	lea	esi,lTMasksDWCnts		;ESI->starting plane's counts

AWBPlane:
	lea	edi,doubleWordCnt 		;EDI->first common counter
	mov	ecx,MASK_AND_COUNT 		;ECX=number of variables for each plane
	rep	movsd				;Restore the five counters for this plane
	cmp	ah,bh				;Check for image width being less than 4 planes
	jne	AWBSetWritePlane		;Blit this plane

	mov	ah,bl				;AH=starting plane's bit mask actual value and wrap around flag
	mov	planeBitMask,ah			;Reset to starting plane's bit mask actual value and wrap around flag 
	jmp	AWBExitChk			;Continue with wipe blit on next line

AWBSetWritePlane:
	mov	pLTMaskDWCnt,esi		;Save pointer to next plane's mask and count
	mov	al,MAP_MASK			;AL=plane map mask register index
	mov	dx,SC_INDEX			;DX=sequence controller index register selector
	out	dx,ax				;Setup for writing to the proper plane for these bytes
	mov	esi,pImageData			;Restore pointer to first image byte of current line in this plane
	mov	edi,pVideoData			;Restore pointer to first video byte of current line in this plane
	mov	edx,lBLWTWTBMask 		;DL bit 0=trailing byte flag:bit 1=trailing word flag:bit 2=leading word flag:bit 3=leading byte flag:bit 4=trailing byte flag:bit 5=trailing word flag:bit 6=leading word flag:bit 7=leading byte flag
	rol	dl,1				;Carry=leading byte flag; DL bit 0=leading byte flag:bit 1=trailing byte flag:bit 2=trailing word flag:bit 3=leading word flag:bit 4=leading byte flag:bit 5=trailing byte flag:bit 6=trailing word flag:bit 7=leading word flag
	jnc	AWBLeadingWord			;No carry - no leading byte

	mov	al,[esi]			;AL=byte from image buffer line
	lea	esi,[esi+4]			;ESI->next byte in image buffer for current plane's line of video buffer
	mov	[edi],al			;Move byte from image buffer to video buffer
	lea	edi,[edi+1]			;EDI->next word in video buffer for current plane's line

AWBLeadingWord:
	rol	dl,1				;Carry=leading word flag; DL bit 0=leading word flag:bit 1=leading byte flag:bit 2=trailing byte flag:bit 3=trailing word flag:bit 4=leading word flag:bit 5=leading byte flag:bit 6=trailing byte flag:bit 7=trailing word flag
	jnc	AWBDoubleWords			;No carry - no leading word

	mov	ah,[esi+4]			;AH=byte from image buffer
	lea	esi,[esi+8]			;ESI->next byte in image buffer for current plane's line of video buffer
	mov	al,[esi-8]			;AL=byte from image buffer
	lea	edi,[edi+2]			;EDI->next double word in video buffer for current plane's line
	mov	[edi-2],ax			;Move word from image buffer to video buffer

AWBDoubleWords:
	mov	ecx,doubleWordCnt 		;ECX=number of double words in current plane's line
	jecxz	AWBTrailingWord			;ECX is 0 - there are no double words

AWBNextDoubleWord:
	mov	ah,[esi+8]			;AH=byte from image buffer
	dec	ecx				;ECX=number of double words remaining after this one
	mov	al,[esi+12]			;AL=byte from image buffer
	bswap	eax				;EAL=AH; EAH=AL
	lea	esi,[esi+16]			;ESI->next byte in image buffer for current plane's line of video buffer
	mov	ah,[esi-12]			;AH=byte from image buffer
	lea	edi,[edi+4]			;EDI->next double word in video buffer
	mov	al,[esi-16]			;AL=byte from image buffer
	mov	[edi-4],eax			;Move double word image data to video buffer
	jnz	AWBNextDoubleWord 		;Loop until all double words have been moved

AWBTrailingWord:
	rol	dl,1				;Carry=trailing word flag; DL bit 0=trailing word flag:bit 1=leading word flag bit 2=leading byte flag:bit 3=trailing byte flag:bit 4=trailing word flag:bit 5=leading word flag:bit 6=leading byte flag:bit 7=trailing byte flag
	jnc	AWBTrailingByte			;No carry - no trailing word

	mov	ah,[esi+4]			;AH=byte from image buffer
	lea	esi,[esi+8]			;ESI->next byte in image buffer for current plane's line of video buffer
	mov	al,[esi-8]			;AL=byte from image bu fer
	lea	edi,[edi+2]			;EDI->next byte in video buffer for current plane's line
	mov	[edi-2],ax			;Move word from image buffer to video buffer

AWBTrailingByte:
	rol	dl,1				;Carry=trailing byte flag; DL bit 0=trailing byte flag:bit 1=trailing word flag:bit 2=leading word flag bit 3=leading byte flag:bit 4=trailing byte flag:bit 5=trailing word flag:bit 6=leading word flag:bit 7=leading byte flag
	jnc	AWBNextPlane			;No carry - no trailing byte

	mov	al,[esi]			;AL=byte from image buffer
	mov	[edi],al			;Move byte from image buffer to video buffer

AWBNextPlane:
	mov	ah,planeBitMask			;AH=previous plane's bit mask actual value and wrap around flag
	inc	DWORD PTR pImageData 		;Set image buffer offset to first byte of next plane
	rol	ah,1				;AH=this plane's bit mask actual value and wrap around flag
	jnc	AWBNextLineChk 			;Blit plane if plane number didn't wrap to 0

	inc	DWORD PTR pVideoData 		;Set video buffer offset to reflect plane 0 wrap around

AWBNextLineChk:
	cmp	ah,bl				;Check for wrap around to starting plane
	mov	esi,pLTMaskDWCnt		;ESI->this plane's mask and count
	mov	planeBitMask,ah			;Save this plane's bit mask actual value and wrap around flag
	jne	AWBPlane			;Not starting plane - move image data into next plane

AWBExitChk:
	dec	DWORD PTR bufferHeight 		;Update number of lines remaining to blit
	jz	ABExit				;Blit is complete when all lines have been blitted

	mov	esi,pImageBuffer		;ESI->start of this line in rectangle of image buffer
	mov	edi,pVideoBuffer		;EDI->start of this line in rectangle of video buffer
	add	esi,bufferSpan			;ESI->start of next line in rectangle of image buffer
	lea	edi,[edi+VIDEO_BUFFER_WIDTH] 	;EDI->start of next line in rectangle of video buffer
	mov	pImageData,esi			;Save pointer to next byte of image buffer
	mov	pVideoData,edi			;Save pointer to next byte of video buffer
	mov	pImageBuffer,esi		;Save pointer to next byte of image buffer
	mov	pVideoBuffer,edi		;Save pointer to next byte of video buffer
	lea	esi,lTMasksDWCnts		;ESI->starting plane's mask and count
	jmp	AWBPlane 			;Blit this plane's line

ABlit_		endp

;---[ AFBlit ]-----------------------------------------
;
; Blit 320 x 400 flic image data to 320 x 400 video buffer
; Parameters:	EAX = pointer to image data
;		EDX = video segment address
; Returns:	Nothing
;
;--------------------------------------------------------------------

PLANE_0_MASK	equ	0x11h			;Plane 0 bit mask
BYTES_PER_FRAME	equ	(SCREEN_WIDTH * SCREEN_HEIGHT)
BYTES_PER_PLANE	equ	(SCREEN_WIDTH / 4 * SCREEN_HEIGHT)

AFBlit_		proc
	push	esi				;Save ESI on stack
	push	edi				;Save EDI on stack
	mov	esi,eax				;ESI->source
	mov	edi,edx				;EDI->destination
	mov	bl,MAP_MASK			;BL=plane map mask register index
	mov	bh,PLANE_0_MASK			;BH=plane 0 bit mask and wrap around flag
	mov	dx,SC_INDEX			;DX=sequence controller index register selector

AFBPlane:
	mov	ax,bx
	out	dx,ax				;Setup for writing to the proper plane for these bytes
	mov	ecx,BYTES_PER_PLANE		;ECX=number of byte quads in each plane

AFBNextByte:
	mov	al,[esi]
	lea	edi,[edi+1]
	dec	ecx				;ECX=number of double word quads remaining to blit
	mov	[edi-1],al
	lea	esi,[esi+4]
	jnz	AFBNextByte			;Loop until all bytes have been moved

	lea	esi,[esi-BYTES_PER_FRAME+1]	;ESI->next byte in image buffer for next plane's line of video buffer
	lea	edi,[edi-BYTES_PER_PLANE]	;EDI->first double word in video buffer
	rol	bh,1				;AH=this plane's bit mask actual value and wrap around flag
	jnc	AFBPlane 			;Blit plane if plane number didn't wrap to 0

AFBExit:
	pop	edi				;Restore EDI from stack
	pop	esi				;Restore ESI from stack

	ret

AFBlit_		endp

;---[ ARBlit ]-----------------------------------------
;
; Blit 320 x 400 video buffer data to image buffer
; Parameters:	EAX = driver (not currently used)
;		EBX = X coordinate
;		ECX = Y coordinate
;		EDX = pointer to image data
;		stack+10h = image buffer width
;		stack+14h = image buffer height
;		stack+18h = video segment (not currently used)
;			    (if used in the future, you must use var. ARBlitType
; Returns:	Nothing
;
;--------------------------------------------------------------------

ARBlit_		proc
	push	esi				;Save ESI on stack
	push	edi				;Save EDI on stack
	push	ebp				;Save EBP on stack
	mov	ebp,esp				;EBP->bottom of stack frame
	sub	esp,STK_FRAME_SIZE		;ESP->top of stack frame
	mov	esi,ecx				;ESI=Y coordinate
	mov	ecx,STK_FRAME_SIZE/4 		;ECX=number of double words in stack frame
	xor	eax,eax				;EAX=0
	lea	edi,firstLocalVar 		;EDI->first local variable in stack frame
	cld					;Clear direction flag
	rep	stosd				;Set all local variables to 0
	mov	edi,esi				;EDI=Y coordinate
	shl	edi,6				;EDI=Y coordinate * 64
	shl	esi,4				;ESI=Y coordinate * 16
	add	esi,edi				;ESI=Y coordinate * 80 (line's offset into video buffer)
	add	esi,_pVGAMem			;ESI->start of line in video buffer
	mov	edi,ebx				;EDI=X coordinate
	shr	edi,2				;EDI=X coordinate / 4 (byte's offset into video buffer line)
	add	esi,edi				;ESI->top-left byte of source rectangle in video buffer
	mov	edi,edx				;EDI->top-left byte of destination rectangle in image buffer
	mov	eax,bufferWidth			;EAX=width of image rectangle
	mov	planeWidth,eax 			;Save plane width of image rectangle
	mul	DWORD PTR bufferHeight 		;EAX=total number of bytes to blit
	add	bytesToBlit,eax			;bytesToBlit=total number of bytes to blit
	jz	ARBExit				;EAX is 0 - no bytes to blit

 	and	ebx,3				;EBX=X coordinate mod 4 (starting plane's plane number)
	mov	ah,bl				;AH=starting plane's plane number

ARBPlane:
	mov	planeNumber,ah 			;Save this plane's plane number
	mov	pVideoBuffer,esi 		;Save pointer to start of current line in source rectangle's plane of video buffer
	mov	pImageBuffer,edi 		;Save pointer to first image byte for current plane's line
	mov	al,READ_MAP			;AL=read map (plane) register index
	mov	dx,GC_INDEX			;DX=graphics controller index register selector
	out	dx,ax				;Setup for reading from the proper plane for these bytes
	mov	ecx,planeWidth 			;ECX=plane width of image rectangle 
	add	ecx,3				;ECX=extend width to a full 4 plane set
	shr	ecx,2				;ECX=number of bytes in each line of this plane
	mov	eax,bufferHeight 		;EAX=height of destination rectangle
	mul	ecx				;EAX=number of bytes to move in this plane
	sub	bytesToBlit,eax			;bytesToBlit=number of bytes remaining to move after this plane
	mov	edx,ecx 			;EDX=number of bytes in each line of this plane
	mov	ebx,bufferHeight 		;EBX=height of destination rectangle
	cmp	BYTE PTR useByteBlit,1		;Check for word & double word blit capability
	je	ARBByteBlit			;Doesn't have word & double word blit capability

	mov	ecx,esi				;ECX=address of first byte in this line
	and	ecx,1				;ECX=number of bytes to word boundary
	xor	al,al				;AL=0
	sub	al,cl				;AL=FFh or 00h
	and	al,LEADING_BYTE			;AL bit 3=leading byte flag
	sub	edx,ecx				;EDX=number of bytes remaining in line
	jz	ARBSetLTBMask			;No bytes remaining to move

	add	ecx,esi				;ECX=address of first word in this line
	and	ecx,2				;ECX=number of bytes to double word boundary
	cmp	edx,ecx				;EDX=number of bytes remaining in line
	jl	ARBSetTrailingFlags		;No double words or trailing words left to move

	xor	ah,ah				;AH=0
	sub	ah,cl				;AH=FEh or 00h
	and	ah,LEADING_WORD			;AH bit 2=leading word flag
	or	al,ah				;AL bit 2=leading word flag
	sub	edx,ecx				;EDX=number of bytes remaining in line
	jz	ARBSetLTBMask			;No bytes remaining to move

	mov	ecx,edx 			;ECX=number of bytes remaining in line
	shr	ecx,2				;ECX=number of double words in line
	mov	doubleWordCnt,ecx 		;Save double word count
	shl	ecx,2				;ECX=double word count converted to bytes
	sub	edx,ecx				;EDX=number of bytes remaining in line

ARBSetTrailingFlags:
	mov	ecx,edx				;ECX=number of bytes remaining after double words
	and	ecx,TRAILING_WORD		;ECX=number of trailing words
	or	al,cl				;AL bit 1=trailing word flag
	and	edx,TRAILING_BYTE		;EDX=number of bytes remaining in line
	or	al,dl				;AL bit 0=trailing byte flag:bit 1=trailing word flag:bit 2=leading word flag:bit 3=leading byte flag

ARBSetLTBMask:
	mov	dl,al				;DL bit 0=trailing byte flag:bit 1=trailing word flag:bit 2=leading word flag:bit 3=leading byte flag
	shl	al,4				;AL bit 4=trailing byte flag:bit 5=trailing word flag:bit 6=leading word flag:bit 7=leading byte flag
	or	dl,al				;DL bit 0=trailing byte flag:bit 1=trailing word flag:bit 2=leading word flag:bit 3=leading byte flag:bit 4=trailing byte flag:bit 5=trailing word flag:bit 6=leading word flag:bit 7=leading byte flag

ARBLine:
	mov	pVideoData,esi			;Save pointer to start of this line in video buffer
	mov	pImageData,edi			;Save pointer to first image byte for current line
	rol	dl,1				;Carry=leading byte flag; DL bit 0=leading byte flag:bit 1=trailing byte flag:bit 2=trailing word flag:bit 3=leading word flag:bit 4=leading byte flag:bit 5=trailing byte flag:bit 6=trailing word flag:bit 7=leading word flag
	jnc	ARBLeadingWord			;No carry - no leading byte

	mov	al,[esi]			;AL=byte from video buffer
	lea	esi,[esi+1]			;ESI->next word in video buffer for current plane's line of video buffer
	mov	[edi],al			;Move byte from video buffer to image buffer
	lea	edi,[edi+4]			;EDI->next byte in image buffer for current plane's line

ARBLeadingWord:
	rol	dl,1				;Carry=leading word flag; DL bit 0=leading word flag:bit 1=leading byte flag:bit 2=trailing byte flag:bit 3=trailing word flag:bit 4=leading word flag:bit 5=leading byte flag:bit 6=trailing byte flag:bit 7=trailing word flag
	jnc	ARBDoubleWords			;No carry - no leading word

	mov	ax,[esi]			;AX=word from video buffer
	lea	esi,[esi+2]			;ESI->next double word in video buffer for current plane's line of video buffer
	mov	[edi+4],ah			;Move byte from video buffer to image buffer
	lea	edi,[edi+8]			;EDI->next byte in image buffer for current plane's line
	mov	[edi-8],al			;Move byte from video buffer to image buffer

ARBDoubleWords:
	mov	ecx,doubleWordCnt 		;ECX=number of double words in current plane's line
	jecxz	ARBTrailingWord			;ECX is 0 - there are no double words

ARBNextDoubleWord:
	mov	eax,[esi]			;EAX=double word from video buffer
	dec	ecx				;ECX=number of double words remaining after this pair
	mov	[edi+4],ah			;Move byte from video buffer to image buffer
	mov	[edi],al			;Move byte from video buffer to image buffer
	lea	esi,[esi+4]			;ESI->next double word byte group in image buffer for current plane's line of video buffer
	bswap	eax				;AH=EAL; AL=EAH
	lea	edi,[edi+16]			;EDI->next byte in image buffer for current plane's line of video buffer
	mov	[edi-8],ah			;Move byte from video buffer to image buffer
	mov	[edi-4],al			;Move byte from video buffer to image buffer
	jnz	ARBNextDoubleWord 		;Loop until all double words have been moved

ARBTrailingWord:
	rol	dl,1				;Carry=trailing word flag; DL bit 0=trailing word flag:bit 1=leading word flag bit 2=leading byte flag:bit 3=trailing byte flag:bit 4=trailing word flag:bit 5=leading word flag:bit 6=leading byte flag:bit 7=trailing byte flag
	jnc	ARBTrailingByte			;No carry - no trailing word

	mov	ax,[esi]			;AX=word from video buffer
	lea	esi,[esi+2]			;ESI->next byte in video buffer for current plane's line
	mov	[edi+4],ah			;Move byte from video buffer to image buffer
	lea	edi,[edi+8]			;EDI->next byte in image buffer for current plane's line of video buffer
	mov	[edi-8],al			;Move byte from video buffer to image buffer

ARBTrailingByte:
	rol	dl,1				;Carry=trailing byte flag; DL bit 0=trailing byte flag:bit 1=trailing word flag:bit 2=leading word flag bit 3=leading byte flag:bit 4=trailing byte flag:bit 5=trailing word flag:bit 6=leading word flag:bit 7=leading byte flag
	jnc	ARBFinishedChk			;No carry - no trailing byte

	mov	al,[esi]			;AL=byte from video buffer
	mov	[edi],al			;Move byte from video buffer to image buffer

ARBFinishedChk:	
	dec	ebx				;Update number of plane lines remaining to blit
	jz	ARBNextPlane			;Plane blit is complete when EBX is 0

	mov	esi,pVideoData			;ESI->start of previous line in source rectangle of video buffer
	mov	edi,pImageData			;EDI->first image byte for previous line
	lea	esi,[esi+VIDEO_BUFFER_WIDTH] 	;ESI->start of this line in source rectangle of video buffer
	add	edi,bufferWidth			;EDI->first image byte for this line
	cmp	BYTE PTR useByteBlit,1		;Check for word & double word blit capability
	jne	ARBLine				;Has word & double word blit capability

ARBByteBlit:
	mov	pVideoData,esi 			;Save pointer to start of current line in source rectangle's plane of video buffer
	mov	pImageData,edi 			;Save pointer to first image byte for current plane's line
	mov	ecx,edx				;ECX=number of bytes in each line of this plane

ARBByteByByte:
	mov	al,[esi]			;AL=byte from video buffer
	dec	ecx				;ECX=number of bytes remaining after this one
	mov	[edi],al			;Move byte from video buffer to image buffer
	lea	esi,[esi+1]			;ESI->next byte in video buffer for current plane's line
	jz	ARBFinishedChk			;ECX is 0 when all bytes in current plane's line have been moved

	lea	edi,[edi+4]			;EDI->next byte in image buffer for current plane's line of video buffer
	jmp	ARBByteByByte			;Loop until all bytes have been moved 

ARBNextPlane:
	mov	ecx,bytesToBlit			;Check number of bytes to blit count
	jecxz	ARBExit				;Blit is complete when bytesToBlit is 0

	mov	DWORD PTR doubleWordCnt,0 	;Clear double word count
	dec	DWORD PTR planeWidth		;Update plane width of plane rectangle
	mov	esi,pVideoBuffer		;ESI->first video byte for this plane
	mov	ah,planeNumber			;AH=previous plane's plane number
	mov	edi,pImageBuffer		;EDI->first image byte for previous plane's line
	inc	ah				;AH=this plane's plane number
	lea	edi,[edi+1]			;EDI->first image byte for this plane's line
	and	ah,3				;AH=this plane's plane number mod 4
	jnz	ARBPlane 			;Blit plane if plane number didn't wrap to 0

	lea	esi,[esi+1]			;Set video buffer offset to reflect plane 0 wrap around
	jmp	ARBPlane 			;Blit plane 0 

ARBExit:
	mov	esp,ebp				;Restore stack pointer
	pop	ebp				;Restore EBP from stack
	pop	edi				;Restore EDI from stack
	pop	esi				;Restore ESI from stack

	ret	ARB_STACK_ARG_BYTES		;Return to caller
ARBlit_		endp


;---[ SyncWithVBL ]-----------------------------------------

INPUT_STATUS_1_REG	equ	3DAh

SyncWithVBL_	proc

	push	eax				;Save EAX on stack
	push	edx				;Save EDX on stack

	mov	dx,INPUT_STATUS_1_REG		;Set VGA index to Input Status #1 Register

WaitForVBlank:				
	in	al,dx				;Get state of vertical retrace
	and	al,8				;AL=state vertical retrace is in
	jz	WaitForVBlank			;AL is zero when vertical retrace is inactive

	pop	edx				;Restore EDX from stack
	pop	eax				;Restore EAX from stack

	ret

SyncWithVBL_	endp


_OSCODE	ENDS
			END
