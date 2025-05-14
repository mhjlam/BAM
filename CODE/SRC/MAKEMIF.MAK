# MAKEMIF makefile

PNAME		= MAKEMIF
OBJS = &
	makemif.obj &
	writeres.obj

ENGINE_PATH	= c:\BAM\tigre
LIB	= 	$(ENGINE_PATH)\tigre.lib

DEBUG_CFLAGS	=	/c /j /zq /w3 /d3 /3r
AFLAGS			=	-3r -mf -d1 -zq
CFLAGS			=	/c /j /zq /w3 /d1 /3r


#====================================================================

$(PNAME).exe: $(OBJS) $(PNAME).lnk $(ENGINE_PATH)\tigre.lib
	wlink @$(PNAME).lnk NAME $(PNAME).exe

#====================================================================

makemif.obj : makemif.cpp
	wcl386 $(DEBUG_CFLAGS) $*

writeres.obj : writeres.cpp
	wcl386 $(DEBUG_CFLAGS) $*

.cpp.obj: .AUTODEPEND
	wcl386 $(DEBUG_CFLAGS) $*

.asm.obj: .AUTODEPEND
	wasm $(AFLAGS) $*

#====================================================================

$(PNAME).lnk : $(PNAME).mak
	echo. > $(PNAME).lnk 
	echo DEBUG ALL >> $(PNAME).lnk
	for %i in ($(OBJS)) do echo file %i >> $(PNAME).lnk
	echo OPTION MAP >> $(PNAME).lnk
	echo LIBRARY $(LIB) >> $(PNAME).lnk
	echo OPTION STACK=8k >> $(PNAME).lnk

