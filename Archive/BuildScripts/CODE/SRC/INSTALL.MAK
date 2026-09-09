#====================================================================
#
#	Install basic makefile
#
#====================================================================

PNAME = 	install

GAME_OBJS = install.obj&
	    tigre\dpmi.obj
	    
			
###  	directory which contains tigre.lib & tigre includes
#T_DIR = 	c:\watcom\lib386\dos
#LIB	= 	graph.lib, clib3r.lib

OFLAGS			=	/c /j /s /zq /w3 /4r

# JCrane way of doing it
CFLAGS			=	/c /j /zq /w3 /d1 /3r /omaxt

DEBUG_CFLAGS	=	/c /j /zq /w9 /d2 /3r
AFLAGS			=	-3r -mf


#====================================================================

$(PNAME).exe: $(GAME_OBJS) $(PNAME).lnk
	wlink @$(PNAME).lnk NAME $(PNAME).exe

#====================================================================
# with debug information

install.obj : install.cpp
	wcl386 $(OFLAGS) $*
#	wcl386 $(DEBUG_CFLAGS) $*

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
	for %i in ($(GAME_OBJS)) do echo file %i >> $(PNAME).lnk
	echo OPTION MAP >> $(PNAME).lnk
	echo LIBPATH $(T_DIR) >> $(PNAME).lnk
	echo LIBRARY $(LIB) >> $(PNAME).lnk
	echo OPTION STACK=8k >> $(PNAME).lnk





