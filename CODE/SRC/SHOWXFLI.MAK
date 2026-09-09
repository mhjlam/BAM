#====================================================================
#
#	Showxfli basic makefile
#
#====================================================================

PNAME = 	showxfli

APP_OBJS = 	showxfli.obj	&
		flic.obj
		
T_DIR = 	c:\bam\tigre

LIB	= 	$(T_DIR)\tigre.lib

CFLAGS			=	/c /j /zq /w3 /d1 /3r /omaxt
DEBUG_CFLAGS	=	/c /j /zq /w3 /d2 /3r
AFLAGS			=	-3r -mf

#====================================================================

$(PNAME).exe: $(APP_OBJS) $(PNAME).lnk $(T_DIR)\tigre.lib
	wlink @$(PNAME).lnk NAME $(PNAME).exe

#====================================================================
# with debug information

flic.obj : flic.cpp
	wcl386 $(DEBUG_CFLAGS) $*

showxfli.obj : showxfli.cpp
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
	echo LIBRARY $(LIB) >> $(PNAME).lnk
	echo OPTION STACK=8k >> $(PNAME).lnk
