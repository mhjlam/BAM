#====================================================================
#
#	MAKESERL basic makefile
#
#====================================================================

PNAME = 	MAKESERL

POBJS = &
		makeserl.obj
			

CFLAGS			=	/c /j /zq /w3 /d1 /3r
#CFLAGS			=	/c /j /zq /w3 /d1 /3r /od
DEBUG_CFLAGS	=	/c /j /zq /w9 /d2 /3r
AFLAGS			=	-3r -mf


#====================================================================

$(PNAME).exe: $(POBJS) $(PNAME).lnk
	wlink @$(PNAME).lnk NAME $(PNAME).exe

#====================================================================
# with debug information

makeserl.obj : makeserl.cpp
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
	for %i in ($(POBJS)) do echo file %i >> $(PNAME).lnk
	echo OPTION MAP >> $(PNAME).lnk
	echo LIBPATH $(T_DIR) >> $(PNAME).lnk
	echo LIBRARY $(LIB) >> $(PNAME).lnk
	echo OPTION STACK=8k >> $(PNAME).lnk





