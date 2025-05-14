#====================================================================
#
#	MAKEWPIF makefile
#
#====================================================================

PNAME = 	MAKEWPIF

APP_OBJS = &
		MAKEWPIF.obj
CFLAGS			=	/c /j /zq /w3 /d1 /3r /omaxt
DEBUG_CFLAGS		=	/c /j /zq /w9 /d2 /3r
AFLAGS			=	-3r -mf


#====================================================================

$(PNAME).exe: $(APP_OBJS) $(PNAME).lnk
	wlink @$(PNAME).lnk NAME $(PNAME).exe

#====================================================================
# with debug information

MAKEWPIF.obj : MAKEWPIF.cpp
	wcl386 $(DEBUG_CFLAGS) $*


#====================================================================
# default without debug information

.cpp.obj: .AUTODEPEND
	wcl386 $(CFLAGS) $*

.asm.obj: .AUTODEPEND
	wasm $(AFLAGS) $*
	
#====================================================================

$(PNAME).lnk : $(PNAME).mak
	echo. > $(PNAME).lnk 
	echo DEBUG ALL >> $(PNAME).lnk
	for %i in ($(APP_OBJS)) do echo file %i >> $(PNAME).lnk
	echo OPTION MAP >> $(PNAME).lnk
	echo OPTION STACK=8k >> $(PNAME).lnk





