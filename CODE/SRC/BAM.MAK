#====================================================================
#
#	Bam basic makefile
#
#====================================================================

PNAME = 	bam

GAME_OBJS = &
		ai.obj &
		assess.obj &
		bam.obj &
		bam_dg.obj &
		bamfuncs.obj &
		bamfunc2.obj &
		bamguy.obj &
		bampopup.obj &
		bamroom.obj &
		cine.obj &
		choose.obj &
		credits.obj &
		death.obj &
		encyclo.obj &
		encymenu.obj &
		fade.obj &
		flicsmk.obj &
		fmt_lbm.obj &
		hall.obj &
                introhal.obj &
		items.obj &
		legendop.obj &
		mainmenu.obj &
		makechar.obj &
		maps.obj &
		mapsbase.obj &
		netchar.obj &
		nethall.obj &
		netstory.obj &
		option.obj &
		option2.obj &
		option3.obj &
		pather.obj &
		savemenu.obj &
		snap.obj &
		spendexp.obj &
		story.obj &
		tutorial.obj &
		units.obj &
		unitlib.obj &
		viewport.obj &
		winlose.obj &
		winlose2.obj &
		world.obj &
		worldmap.obj
			
###  	directory which contains tigre.lib & tigre includes
T_DIR = 	c:\bam\tigre
LIB	= 	tigre.lib, sosdw1cr, sosmw1cr, netnowr.lib, cpfr32.lib, smack.lib

#old flags
#CFLAGS			=	/c /j /zq /w3 /d1 /3r
#CFLAGS			=	/c /j /zq /w3 /d1 /3r /od
OFLAGS			=	/c /j /s /zq /w3 /4r /fm

# JCrane way of doing it
CFLAGS			=	/c /j /zq /w3 /d1 /3r /omaxt /fm

DEBUG_CFLAGS	=	/c /j /zq /w9 /d2 /3r /fm
AFLAGS			=	-3r -mf /fm


#====================================================================

$(PNAME).exe: $(GAME_OBJS) $(PNAME).lnk
	wlink @$(PNAME).lnk NAME $(PNAME).exe

#====================================================================
# with debug information

ai.obj : ai.cpp
#	wcl386 $(OFLAGS) $*
	wcl386 $(DEBUG_CFLAGS) $*

assess.obj : assess.cpp
	wcl386 $(OFLAGS) $*
#	wcl386 $(DEBUG_CFLAGS) $*

bam.obj : bam.cpp
#	wcl386 $(OFLAGS) $*
	wcl386 $(DEBUG_CFLAGS) $*

bam_dg.obj : bam_dg.cpp
	wcl386 $(OFLAGS) $*

bamfuncs.obj : bamfuncs.cpp
	wcl386 $(OFLAGS) $*

bamfunc2.obj : bamfuncs.cpp
	wcl386 $(OFLAGS) $*

bamroom.obj : bamroom.cpp
	wcl386 $(OFLAGS) $*
#	wcl386 $(DEBUG_CFLAGS) $*

#cine.obj : cine.cpp
	wcl386 $(OFLAGS) $*
#	wcl386 $(DEBUG_CFLAGS) $*

#choose.obj : choose.cpp
	wcl386 $(OFLAGS) $*
#	wcl386 $(DEBUG_CFLAGS) $*

death.obj : death.cpp
	wcl386 $(OFLAGS) $*
#	wcl386 $(DEBUG_CFLAGS) $*

encymenu.obj : encymenu.cpp
	wcl386 $(OFLAGS) $*
#	wcl386 $(DEBUG_CFLAGS) $*

fade.obj : fade.cpp
	wcl386 $(OFLAGS) $*
#	wcl386 $(DEBUG_CFLAGS) $*

flicsmk.obj : flicsmk.cpp
	wcl386 $(DEBUG_CFLAGS) $*

hall.obj : hall.cpp
	wcl386 $(OFLAGS) $*
#	wcl386 $(DEBUG_CFLAGS) $*

intro.obj : intro.cpp
	wcl386 $(OFLAGS) $*
#	wcl386 $(DEBUG_CFLAGS) $*

items.obj : items.cpp
#	wcl386 $(OFLAGS) $*
	wcl386 $(DEBUG_CFLAGS) $*

makechar.obj : makechar.cpp
	wcl386 $(OFLAGS) $*
#	wcl386 $(DEBUG_CFLAGS) $*

mainmenu.obj : mainmenu.cpp
	wcl386 $(OFLAGS) $*
#	wcl386 $(DEBUG_CFLAGS) $*

maps.obj : maps.cpp
#	wcl386 $(OFLAGS) $*
	wcl386 $(DEBUG_CFLAGS) $*

mapsbase.obj : mapsbase.cpp
#	wcl386 $(OFLAGS) $*
	wcl386 $(DEBUG_CFLAGS) $*

netstory.obj : netstory.cpp
	wcl386 $(OFLAGS) $*
#	wcl386 $(DEBUG_CFLAGS) $*

option.obj : option.cpp
	wcl386 $(OFLAGS) $*
#	wcl386 $(DEBUG_CFLAGS) $*

option2.obj : option2.cpp
	wcl386 $(OFLAGS) $*
#	wcl386 $(DEBUG_CFLAGS) $*

option3.obj : option3.cpp
	wcl386 $(OFLAGS) $*
#	wcl386 $(DEBUG_CFLAGS) $*

pather.obj : pather.cpp
	wcl386 $(OFLAGS) $*
#	wcl386 $(DEBUG_CFLAGS) $*

savemenu.obj : savemenu.cpp
	wcl386 $(OFLAGS) $*
#	wcl386 $(DEBUG_CFLAGS) $*

story.obj : story.cpp
	wcl386 $(OFLAGS) $*
#	wcl386 $(DEBUG_CFLAGS) $*

tutorial.obj : tutorial.cpp
	wcl386 $(OFLAGS) $*

units.obj : units.cpp
#	wcl386 $(OFLAGS) $*
	wcl386 $(DEBUG_CFLAGS) $*

unitlib.obj : unitlib.cpp
#	wcl386 $(OFLAGS) $*
	wcl386 $(DEBUG_CFLAGS) $*

viewport.obj : viewport.cpp
#	wcl386 $(OFLAGS) $*
	wcl386 $(DEBUG_CFLAGS) $*

winlose.obj : winlose.cpp
	wcl386 $(OFLAGS) $*
#	wcl386 $(DEBUG_CFLAGS) $*

winlose2.obj : winlose2.cpp
	wcl386 $(OFLAGS) $*
#	wcl386 $(DEBUG_CFLAGS) $*

world.obj : world.cpp
#	wcl386 $(OFLAGS) $*
	wcl386 $(DEBUG_CFLAGS) $*

worldmap.obj : worldmap.cpp
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





