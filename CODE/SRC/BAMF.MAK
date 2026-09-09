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
	wcl386 $(OFLAGS) $*

assess.obj : assess.cpp
	wcl386 $(OFLAGS) $*

bam.obj : bam.cpp
	wcl386 $(OFLAGS) $*

bam_dg.obj : bam_dg.cpp
	wcl386 $(OFLAGS) $*

bamfuncs.obj : bamfuncs.cpp
	wcl386 $(OFLAGS) $*

bamfunc2.obj : bamfuncs.cpp
	wcl386 $(OFLAGS) $*

bamguy.obj : bamguy.cpp
	wcl386 $(OFLAGS) $*

bampopup.obj : bampopup.cpp
	wcl386 $(OFLAGS) $*

bamroom.obj : bamroom.cpp
	wcl386 $(OFLAGS) $*

cine.obj : cine.cpp
	wcl386 $(OFLAGS) $*

choose.obj : choose.cpp
	wcl386 $(OFLAGS) $*

credits.obj : credits.cpp
	wcl386 $(OFLAGS) $*

death.obj : death.cpp
	wcl386 $(OFLAGS) $*

encyclo.obj : encyclo.cpp
	wcl386 $(OFLAGS) $*

encymenu.obj : encymenu.cpp
	wcl386 $(OFLAGS) $*

fade.obj : fade.cpp
	wcl386 $(OFLAGS) $*

flicsmk.obj : flicsmk.cpp
	wcl386 $(OFLAGS) $*

hall.obj : hall.cpp
	wcl386 $(OFLAGS) $*

intro.obj : intro.cpp
	wcl386 $(OFLAGS) $*

items.obj : items.cpp
	wcl386 $(OFLAGS) $*

makechar.obj : makechar.cpp
	wcl386 $(OFLAGS) $*

mainmenu.obj : mainmenu.cpp
	wcl386 $(OFLAGS) $*

maps.obj : maps.cpp
	wcl386 $(OFLAGS) $*

mapsbase.obj : mapsbase.cpp
	wcl386 $(OFLAGS) $*

netstory.obj : netstory.cpp
	wcl386 $(OFLAGS) $*

option.obj : option.cpp
	wcl386 $(OFLAGS) $*

option2.obj : option2.cpp
	wcl386 $(OFLAGS) $*

option3.obj : option3.cpp
	wcl386 $(OFLAGS) $*

pather.obj : pather.cpp
	wcl386 $(OFLAGS) $*

savemenu.obj : savemenu.cpp
	wcl386 $(OFLAGS) $*

story.obj : story.cpp
	wcl386 $(OFLAGS) $*

tutorial.obj : tutorial.cpp
	wcl386 $(OFLAGS) $*

units.obj : units.cpp
	wcl386 $(OFLAGS) $*

unitlib.obj : unitlib.cpp
	wcl386 $(OFLAGS) $*

viewport.obj : viewport.cpp
	wcl386 $(OFLAGS) $*

winlose.obj : winlose.cpp
	wcl386 $(OFLAGS) $*

winlose2.obj : winlose2.cpp
	wcl386 $(OFLAGS) $*

world.obj : world.cpp
	wcl386 $(OFLAGS) $*

worldmap.obj : worldmap.cpp
	wcl386 $(OFLAGS) $*

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





