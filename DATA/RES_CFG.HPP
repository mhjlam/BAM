enum res_t
{
	RES_CEL,			// 0	cel	.
	RES_ANIM,		// 1	ani	.;z:\ani
	RES_PIC,			// 2	pic	.
	RES_MIDI,		// 3	hmp	.;z:\snd	H M
	RES_DAC,			// 4	wav	.;z:\snd	H M
	RES_PAL,			// 5	pal	.;z:\pal
	RES_DATA,		// 6	dat	.	S
	RES_FONT,		// 7	fon	.;z:\fon	
	RES_SQUIB,		// 8	sqb	.;z:\txt
	RES_CONV,		// 9	cnv	.;z:\txt
	RES_DCEL,		// 10	dcl	.			S L
	RES_CHOREO,		// 11	cgf	.
	RES_BNK,			// 12	bnk	.;z:\snd			H L M G
	RES_SYNCH,		// 13	syn	.	H  
	RES_CHUNK,		// 14	chu	.  
	RES_ROUTE,		// 15	rot	.
	RES_SAVE,		// 16	sav	.  H
	RES_FLIC,		// 17	flc	.;z:\flc  H
	RES_TILELIB,	// 18	tlb	.;z:\tiles
	RES_MAP_INFO,	// 19	mif	.;z:\tiles
	RES_STUFF,		// 20	stf	.  
	RES_8TRACK,		// 21	8tr	.  H
	RES_LAST			// 22
};


/*

Format:

	DEFINE			// number extension path [flag [...]]


The above defines are used in the TIGRE source code.  If the defines
themselves are changed, tigre.exe must be rebuilt.  If the attribute info
in the comment is changed, tigre.exe need not be rebuilt.


The attribute information must contain the file extension and path, and may
also contain:

	(G) - use a system grip, not a standard grip (will not get flushed on restore)
	(H) - headerless flag (this resource will not have a res header)
	(L) - a locked flag
	(M) - non-moveable flag
	(S) - saveable flag (these will be saved and restored)

RES_MIDI, RES_DAC, and RES_DATA use these optional flags.


*/
