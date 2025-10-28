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
		flic8.obj &
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
T_DIR = 	..\tigre
LIB	= 	tigre.lib, sosdw1pr, sosmw1pr, netnowr.lib, cpfr32.lib

CFLAGS			=	/c /j /zq /w3 /d1 /3r /omaxt
#CFLAGS			=	/c /j /zq /w3 /d1 /3r /od
DEBUG_CFLAGS	=	/c /j /zq /w9 /d2 /3r
AFLAGS			=	-3r -mf


#====================================================================

$(PNAME).exe: $(GAME_OBJS) $(PNAME).lnk $(T_DIR)\tigre.lib
	wlink @$(PNAME).lnk NAME $(PNAME).exe

#====================================================================
# with debug information

#ai.obj : ai.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#assess.obj : assess.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

bam.obj : bam.cpp
	wcl386 $(DEBUG_CFLAGS) $*

bamfuncs.obj : bamfuncs.cpp
	wcl386 $(DEBUG_CFLAGS) $*

bamfuncs2.obj : bamfuncs2.cpp
	wcl386 $(DEBUG_CFLAGS) $*

#bamguy.obj : bamguy.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#bampopup.obj : bampopup.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#bamroom.obj : bamroom.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

bam_dg.obj : bam_dg.cpp
	wcl386 $(DEBUG_CFLAGS) $*

#cine.obj : cine.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#choose.obj : choose.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#credits.obj : credits.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#death.obj : death.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#encyclo.obj : encyclo.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#encymenu.obj : encymenu.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#fade.obj : fade.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#flic8.obj : flic8.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#fmt_lbm.obj : fmt_lbm.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#hall.obj : hall.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#introhal.obj : introhal.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

items.obj : items.cpp
	wcl386 $(DEBUG_CFLAGS) $*

#legendop.obj : legendop.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#makechar.obj : makechar.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#mainmenu.obj : mainmenu.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

maps.obj : maps.cpp
	wcl386 $(DEBUG_CFLAGS) $*

mapsbase.obj : mapsbase.cpp
	wcl386 $(DEBUG_CFLAGS) $*

#netchar.obj : netchar.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#nethall.obj : nethall.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#netstory.obj : netstory.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#option.obj : option.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#option2.obj : option2.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#option3.obj : option3.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#pather.obj : pather.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#savemenu.obj : savemenu.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#snap.obj : snap.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#spendexp.obj : spendexp.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#story.obj : story.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#tutorial.obj : tutorial.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

units.obj : units.cpp
	wcl386 $(DEBUG_CFLAGS) $*

unitlib.obj : unitlib.cpp
	wcl386 $(DEBUG_CFLAGS) $*

viewport.obj : viewport.cpp
	wcl386 $(DEBUG_CFLAGS) $*

#winlose.obj : winlose.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

#winlose2.obj : winlose2.cpp
#	wcl386 $(DEBUG_CFLAGS) $*

world.obj : world.cpp
	wcl386 $(DEBUG_CFLAGS) $*

#worldmap.obj : worldmap.cpp
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
