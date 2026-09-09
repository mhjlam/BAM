#====================================================================
#
#	Tigre basic makefile
#
#  WATCOM Version
#
#====================================================================

PNAME				=	tigre

#DISPLAY_OBJS	=	svgadisp.obj vesa.obj
DISPLAY_OBJS	=	xmoddisp.obj modex.obj

LIB_OBJS			=		api.obj &
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
						debug.obj &
		  				dialog.obj &
						dpmi.obj &
		  				eventmgr.obj &
		  				fontmgr.obj &
						file.obj	&
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
						serial.obj	&
		  				soundmgr.obj &
		  				stream.obj &
						srle.obj &
		  				t12.obj &
						text.obj &
						trle.obj &
		  				vesa.obj &
		  				vgabuf.obj &
						$(DISPLAY_OBJS)


# optimizing flags, for modules we feel safe about
OFLAGS			=	/c /j /s /zq /w3 /4r

CFLAGS			=	/c /j /s /zq /w3 /d1 /3r
AFLAGS			=	-3r -mf -zq
DEBUG_CFLAGS	=	/c /j /s /zq /w3 /d2 /3r
#DEBUG_CFLAGS	=	/c /j /s /zq /w3 /d2 /3r /omaxt
#
#	/c    = compile only
#	/j    = change char default from unsigned to signed
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

api.obj : api.cpp
	wcl386 $(OFLAGS) $*

apidlg.obj : apidlg.cpp
	wcl386 $(OFLAGS) $*

apievt.obj : apievt.cpp
	wcl386 $(OFLAGS) $*

apifont.obj : apifont.cpp
	wcl386 $(OFLAGS) $*

apigraph.obj : apigraph.cpp
	wcl386 $(OFLAGS) $*

apimem.obj : apimem.cpp
	wcl386 $(OFLAGS) $*

apires.obj : apires.cpp
	wcl386 $(OFLAGS) $*

comm.obj : comm.cpp
	wcl386 $(OFLAGS) $*

commmgr.obj : commmgr.cpp
	wcl386 $(DEBUG_CFLAGS) $*

eventmgr.obj : eventmgr.cpp
	wcl386 $(OFLAGS) $*

fontmgr.obj : fontmgr.cpp
	wcl386 $(OFLAGS) $*
#	wcl386 $(DEBUG_CFLAGS) $*

graphmgr.obj : graphmgr.cpp
#	wcl386 $(OFLAGS) $*
	wcl386 $(DEBUG_CFLAGS) $*

keylist.obj : keylist.cpp
	wcl386 $(OFLAGS) $*

list.obj : list.cpp
	wcl386 $(OFLAGS) $*

mem.obj : mem.cpp
	wcl386 $(OFLAGS) $*

memmgr.obj : memmgr.cpp
	wcl386 $(OFLAGS) $*

object.obj : object.cpp
	wcl386 $(OFLAGS) $*

rect.obj : rect.cpp
	wcl386 $(OFLAGS) $*

resource.obj : resource.cpp
	wcl386 $(OFLAGS) $*

resmgr.obj : resmgr.cpp
#	DO NOT TRY OFLAGS!  WEIRD CRASHES! -Van
	wcl386 $(DEBUG_CFLAGS) $*

scrimage.obj : scrimage.cpp
	wcl386 $(OFLAGS) $*

serial.obj : serial.cpp
	wcl386 $(DEBUG_CFLAGS) $*

vgabuf.obj : vgabuf.cpp
	wcl386 $(OFLAGS) $*

.cpp.obj: .AUTODEPEND
	wcl386 $(DEBUG_CFLAGS) $*

.asm.obj: .AUTODEPEND
	wasm $(AFLAGS) $*

