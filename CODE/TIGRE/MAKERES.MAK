PNAME		=	makeres
OBJS		=	comp.obj &
				config.obj &
				makeres.obj &
				os.obj &
				rle.obj &
				t12.obj &
				toolmem.obj &
				writeres.obj

CFLAGS	=	/c /zq /w3 /d2 /3r


#
# The explicit rule for the link, which says to use
# the xxx.lnk file and name the exe appropriately
#
$(PNAME).exe: $(OBJS) $(PNAME).lnk
	wlink @$(PNAME).lnk NAME $(PNAME).exe

#
# The explicit rule for compiling each file in the list
#
.cpp.obj: .AUTODEPEND
	wcl386 $(CFLAGS) $*

#
# This section creates a xxx.lnk file that contains
# all the object modules
#
$(PNAME).lnk : $(PNAME).mak
	echo. > $(PNAME).lnk 
	echo DEBUG ALL >> $(PNAME).lnk
	for %i in ($(OBJS)) do echo file %i >> $(PNAME).lnk
	echo OPTION MAP >> $(PNAME).lnk
