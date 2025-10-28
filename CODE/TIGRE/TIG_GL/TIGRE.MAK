#====================================================================
#
#	Tigre basic makefile
#
#  WATCOM Version
#
#====================================================================

PNAME		=	tigre

#DISPLAY_OBJS	=	svgadisp.obj vesa.obj
DISPLAY_OBJS	=	xmoddisp.obj modex.obj

LIB_OBJS	= 8track.obj &
			api.obj &
			apidlg.obj &
			apievt.obj &
			apifont.obj &
			apigraph.obj &
			apimem.obj &
			apires.obj &
 			cliprect.obj &
 			clock.obj &
 			comp.obj &
 			comm.obj &
 			commmgr.obj &
 			config.obj &
 			context.obj &
 			dialog.obj &
			dpmi.obj &
 			eventmgr.obj &
 			file.obj &
 			fontmgr.obj &
 			graphmgr.obj &
 			keylist.obj &
 			list.obj &
 			manager.obj &
 			mem.obj &
 			memmgr.obj &
 			mono.obj &
 			mouse.obj &
			mouseint.obj &
			mousescr.obj &
 			object.obj &
 			objmgr.obj &
			os.obj &
			osgrph.obj &
 			palette.obj &
 			periodic.obj &
 			rect.obj &
 			resmgr.obj &
 			resource.obj &
 			savebase.obj &
 			savemgr.obj &
 			sconfig.obj &
 			scrimage.obj &
 			serial.obj &
 			soundmgr.obj &
 			stream.obj &
			srle.obj &
 			t12.obj &
 			text.obj &
			trle.obj &
 			vesa.obj &
			vgabuf.obj &
			$(DISPLAY_OBJS)


CFLAGS			=	/c /j /s /zq /w3 /d1 /3r
AFLAGS			=	-3r -mf -d1 -zq
DEBUG_CFLAGS	=	/c /j /s /zq /w3 /d2 /3r
#
#	/c    = compile only
#	/j    = change char default from unsigned to signed
#	/s    = turn off stack checking
#	/zq	= quiet mode
#	/zp2	= word alignment
#	/w3	= warning level 3
#	/d2	= debug level
#	/3r	= 386 register-passing



#====================================================================

$(PNAME).lib: $(LIB_OBJS)
	echo. > $(PNAME).rsp
	for %i in ($(LIB_OBJS)) do echo -+%i >> $(PNAME).rsp
	wlib /b /c /l /n /p=32 /q $(PNAME).lib @$(PNAME).rsp



#====================================================================

.cpp.obj: 
	wcl386 $(DEBUG_CFLAGS) $*

.asm.obj: 
	wasm $(AFLAGS) $*

