#====================================================================
#
#       BAM Map Editor makefile
#
#====================================================================

PNAME =         MAPEDIT

APP_OBJS =      MAPEDIT.OBJ	&
		MAPSBASE.OBJ	&
		BAMFUNC2.OBJ	&
		BAMGUY.OBJ	&
		BAM_DG.OBJ	&
		WRITERES.OBJ

T_DIR =     c:\bam\tigre
BAM_DIR = 	c:\bam

LIB	= 	tigre.lib, sosdw1pr, sosmw1pr, netnowr.lib, cctew95r.lib

CFLAGS		=	/c /i=$(BAM_DIR);$(T_DIR) /j /zq /w3 /d1 /3r
DEBUG_CFLAGS	=	/c /i=$(BAM_DIR);$(T_DIR) /j /zq /w3 /d2 /3r
AFLAGS		=	-3r -mf


#====================================================================

$(PNAME).exe: $(APP_OBJS) $(PNAME).lnk $(T_DIR)\tigre.lib
	wlink @$(PNAME).lnk NAME C:\BAM\TILES\$(PNAME).exe

#====================================================================

bamfunc2.obj : bamfunc2.cpp
	wcl386 $(DEBUG_CFLAGS) $*

mapedit.obj : mapedit.cpp
	wcl386 $(DEBUG_CFLAGS) $*

mapsbase.obj : mapsbase.cpp
	wcl386 $(DEBUG_CFLAGS) $*

writeres.obj : writeres.cpp
	wcl386 $(DEBUG_CFLAGS) $*

bam_dg.obj : bam_dg.cpp
	wcl386 $(DEBUG_CFLAGS) $*

.cpp.obj:
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
	echo LIBPATH $(T_DIR) >> $(PNAME).lnk
	echo LIBRARY $(LIB) >> $(PNAME).lnk
	echo OPTION STACK=8k >> $(PNAME).lnk
