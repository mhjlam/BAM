//
// MEM.CPP
//
// May 26, 1989
// WATCOM: September 23, 1994  BKH
// (C) 1986-1991 by Walter Bright  All Rights Reserved.
// Modified December 7, 1993 by Tachyon, Inc.
//
//----[]-------------------------------------------------------------


#include	<new.h>
#include	<stdarg.h>
#include <time.h>

#include	"api.hpp"
#include "apigraph.hpp"
#include "apimem.hpp"
#include	"mem.hpp"
#include "memmgr.hpp"
#include "mono.hpp"

#ifndef MEM_NOMEMCOUNT
#define MEM_NOMEMCOUNT	0
#endif

// because we have things in here that may be called during an interrupt,
// turn stack checking off
#pragma off (check_stack)

int mem_inited = 0;		/* != 0 if initialized			*/

static int mem_behavior = MEM_RETNULL;
static int (*fp) (void) = NULL;	/* out-of-memory handler	*/
static int mem_count;		/* # of allocs that haven't been free'd	*/
static int mem_scount;		/* # of sallocs that haven't been free'd */

//#define PRINT	printf(	/* stderr can't be redirected with MS-DOS	*/
#define PRINT	pMono->Out(

#define	DEFAULT_MEM_STR	"Not Set"

#define NDEBUG

/*******************************/

void mem_setexception(MEM_E flag,...)
{
	va_list ap;
	typedef int (*fp_t)(void);

	mem_behavior = flag;
	va_start(ap,flag);
	fp = (mem_behavior == MEM_CALLFP) ? va_arg(ap,fp_t) : 0;
	va_end(ap);

	#if MEM_DEBUG
		ASSERT(0 <= flag && flag <= MEM_RETRY);
	#endif
}



/*************************
 * This is called when we're out of memory.
 * Returns:
 *	1:	try again to allocate the memory
 *	0:	give up and return NULL
 */

int mem_exception()
{  
	return 0;
}



/****************************/

#ifdef MEM_DEBUG

#undef mem_strdup

char *mem_strdup(const char *s)
{
	return mem_strdup_debug(s,__FILE__,__LINE__);
}


char *mem_strdup_debug(const char* s, const char* file, int line)
{
	char *p;

	p = s
		 ? (char *) mem_malloc_debug((unsigned) strlen(s) + 1, file, line, "")
		 : NULL;
	return p ? strcpy(p,s) : p;
}

#else

char *mem_strdup(const char *s)
{
	char *p;

	p = s ? (char *) mem_malloc((unsigned) strlen(s) + 1) : NULL;
	return p ? strcpy(p,s) : p;
}

#endif /* MEM_DEBUG */




/************* C++ Implementation ***************/

#if __cplusplus
extern "C++"
{

int __mem_line;
char *__mem_file;



/********************
 */

void (*_new_handler)(void);


/*****************************
 * Replacement for the standard C++ library operator new().
 */

#undef new
void * operator new(size_t size)
{
	dgrip	p;
	grip	g;

	while (1)
	{	
		if (size == 0) {
			size++;
		}

		ASSERT (mem_inited);

		g = pMemMgr->Malloc (size);
		p = pMemMgr->Deref(g);

		if (p != NULL || _new_handler == NULL) 
		{
			break;
		}

		(*_new_handler)();
	}
	return p;
}

// call this new for classes that need to be in the systems grip area

void * newSys(size_t size)
{
	dgrip	p;
	grip	g;

	while (1)
	{	
		if (size == 0) {
			size++;
		}

		ASSERT (mem_inited);

		g = pMemMgr->SysMalloc (size);
		p = pMemMgr->Deref(g);

		if (p != NULL || _new_handler == NULL) 
		{
			break;
		}

		(*_new_handler)();
	}
	return p;
}



/***********************
 * Replacement for the standard C++ library operator delete().
 */

#undef delete
void operator delete(void *p)
{
	grip	g;

	g = pMemMgr->GetGrip(p);
 
	if (g != INVALID_GRIP) 
	{
		pMemMgr->Free(g);
	}
	else
	{
		if (pMono) 
		{
			// this is a bug probably cause by a the pointers of two
			// automatic objects on the stack, one already released by 
			// not yet cleared by MemMgr::cycle. jc should fix this
			// as it could leak memory and grips (lazy bastard).
			pMono->Out("Free-ing unalloc'ed object");
			pMemMgr->DumpGrip(g);
		}
	}
}



}
#endif



#ifdef MEM_DEBUG

static long mem_maxalloc;		// max # of bytes allocated
static long mem_numalloc;		// current # of bytes allocated

#define BEFOREVAL	0x4F464542L	// value to detect underrun
#define AFTERVAL	0x45544641L	// value to detect overrun


/* The following should be selected to give maximum probability that	*/
/* pointers loaded with these values will cause an obvious crash. On	*/
/* Unix machines, a large value will cause a segment fault.		*/
/* MALLOCVAL is the value to set malloc'd data to.			*/

//#if MSDOS || __OS2__ || __NT__
//	#define BADVAL		0xFF
//	#define MALLOCVAL	0xEE
//#else
	#define BADVAL		0x7A
	#define MALLOCVAL	0xEE
//#endif

/* Disable mapping macros	*/
#undef	mem_malloc
#undef	mem_calloc
#undef	mem_realloc
#undef	mem_free

/* Create a list of all alloc'ed pointers, retaining info about where	*/
/* each alloc came from. This is a real memory and speed hog, but who	*/
/* cares when you've got obscure pointer bugs.				*/

static mem_debug mem_alloclist =
{
	{
		(struct mem_debug *) NULL,
		(struct mem_debug *) NULL,
		NULL,
		11111,
		"",
		0,
		0,
		BEFOREVAL
	},
	(char) AFTERVAL
};

/* Convert from a void *to a mem_debug struct.	*/
#define mem_ptrtodl(p)	((struct mem_debug *) ((char *)p - sizeof(struct mh)))

/* Convert from a mem_debug struct to a mem_ptr.	*/
#define mem_dltoptr(dl)	((void *) &((dl)->data[0]))

#define next		m.Mnext
#define prev		m.Mprev
#define file		m.Mfile
#define line		m.Mline
#define str			m.Mstr
#define clocks		m.Mclocks
#define nbytes		m.Mnbytes
#define beforeval	m.Mbeforeval

/*****************************
 * Set new value of file,line
 */

void mem_setnewfileline(void* ptr, const char* fil, int lin)
{
	 struct mem_debug *dl;

	 dl = mem_ptrtodl(ptr);
	 dl->file = fil;
	 dl->line = lin;
}

/****************************
 * Print out struct mem_debug.
 */

static void _near mem_printdl(struct mem_debug* dl)
{
	// null term string just in case it isn't valid
	dl->str[MEM_STRING_SIZE - 1] = 0;
	PRINT "alloc'd from file '%s' line %d nbytes %d ptr %p name %s\n",
		dl->file,dl->line,dl->nbytes,(long)mem_dltoptr(dl), dl->str);
}

/****************************
 * Print out file and line number.
 */

static void _near mem_fillin(const char* fil, int lin)
{
	PRINT "File '%s' line %d\n",fil,lin);
#ifdef ferr
	fflush(ferr);
#endif
}



/****************************
 * If MEM_DEBUG is not on for some modules, these routines will get
 * called.
 */

void *mem_calloc(uint u)
{
		//return mem_calloc_debug(u,__FILE__,__LINE__);
		return mem_calloc_debug(u, szFileTrack, lineTrack, szStrTrack);
}

void *mem_malloc(uint u)
{
	  	//return mem_malloc_debug(u,__FILE__,__LINE__);
	  	return mem_malloc_debug(u, szFileTrack, lineTrack, szStrTrack);
}

void *mem_realloc(void* p, uint u)
{
	  	//return mem_realloc_debug(p,u,__FILE__,__LINE__);
	  	return mem_realloc_debug(p, u, szFileTrack, lineTrack, szStrTrack);
}

void mem_free(void* p)
{
	//mem_free_debug(p,__FILE__,__LINE__);
	mem_free_debug(p,szFileTrack, lineTrack);
}	 


/**************************/

void mem_freefp(void* p)
{
	mem_free(p);
}



/***********************
 * Debug versions of mem_calloc(), mem_free() and mem_realloc().
 */

void *mem_malloc_debug(uint n, const char* fil, int lin, const char* s)
{
	void *p;

	fil = fil;	// Gets rid of unreferenced warning
	lin = lin;
	s = s;

	p = mem_calloc_debug(n, szFileTrack, lineTrack, szStrTrack);
//	if (p)
//		memset(p,MALLOCVAL,n);

	return p;
}

void *mem_calloc_debug(uint n, const char* fil, int lin, const char* s)
{
	 struct mem_debug *dl;

	do	{
		dl = (struct mem_debug *)
		 //calloc(sizeof(*dl) + n + sizeof(AFTERVAL) - 1,1);
		 malloc(sizeof(*dl) + n + sizeof(AFTERVAL) - 1);
	}

	while (dl == NULL && mem_exception());
	if (dl == NULL)
	{

#if 0
	printf("Insufficient memory for alloc of %d at ",n);
	mem_fillin(fil,lin);
	printf("Max allocated was: %ld\n",mem_maxalloc);
#endif

		return NULL;
	}
	
	dl->file = fil;
	dl->line = lin;
	memset(dl->str, NULL, MEM_STRING_SIZE);
	strncpy(dl->str, s, MEM_STRING_SIZE - 1);
	dl->clocks = clock();

	//*	This test is not compatible with Greenleaf Comm++
//	if (strcmp(dl->str, DEFAULT_MEM_STR) == 0)
//	{
//		APanic("no track mem set");
//	}

	szStrTrack = DEFAULT_MEM_STR;
	dl->nbytes = n;
	dl->beforeval = BEFOREVAL;
	*(long *) &(dl->data[n]) = AFTERVAL;

	/* Add dl to start of allocation list	*/
	dl->next = mem_alloclist.next;
	dl->prev = &mem_alloclist;
	mem_alloclist.next = dl;
	if (dl->next != NULL)
	dl->next->prev = dl;

	mem_count++;
	mem_numalloc += n;
	if (mem_numalloc > mem_maxalloc)
	mem_maxalloc = mem_numalloc;
	return mem_dltoptr(dl);
}



void mem_free_debug(void* ptr, const char* fil, int lin)
{
	struct mem_debug *dl;
	
	if (ptr == NULL)
		return;

	if (mem_count <= 0)
	{
		pMono->Out("More frees than allocs at %s(%d)\n", fil, lin);
		#ifndef NDEBUG
		goto err;
		#endif
	}

	dl = mem_ptrtodl(ptr);
	if (dl->beforeval != BEFOREVAL)
	{
		pMono->Out("Pointer x%lx underrun in %s(%d)\n", (long)ptr, fil, lin);
		#ifndef NDEBUG
		goto err2;
		#endif
	}

	if (*(long *) &dl->data[dl->nbytes] != AFTERVAL)
	{
		pMono->Out("Pointer x%lx overrun in %s(%d)\n", (long)ptr, fil, lin);
		#ifndef NDEBUG
		goto err2;
		#endif
	}

	mem_numalloc -= dl->nbytes;
	if(mem_numalloc < 0)
	{
		pMono->Out("error: mem_numalloc = %ld, dl->nbytes = %d.  %s(%d)\n",
			mem_numalloc, dl->nbytes, fil, lin);
		#ifndef NDEBUG
		goto err2;
		#endif
	}

	/* Remove dl from linked list	*/
	if (dl->prev)
		dl->prev->next = dl->next;
	if (dl->next)
		dl->next->prev = dl->prev;

	/* Stomp on the freed storage to help detect references	*/
	/* after the storage was freed.				*/
	/* memset((void *) dl,BADVAL,sizeof(*dl) + dl->nbytes); */

	mem_count--;

	free((void *) dl);
	return;

err2:
	// back to text mode
	AShutDownVideo();
	// print error data
	mem_printdl(dl);
	pMono->Out("free'd from ");
	mem_fillin(fil,lin);
	ASSERT(0);
	/* NOTREACHED */
	return;
err:
	pMono->Out("free'd from ");
	mem_fillin(fil,lin);
	ASSERT(0);
	/* NOTREACHED */
	return;
}



/*******************
 * Debug version of mem_realloc().
 */

void *mem_realloc_debug(void* oldp, uint n, const char* fil, int lin, const char* s)
{
	void *p;
	struct mem_debug *dl;

	if (n == 0)
	{
		mem_free_debug(oldp,fil,lin);
		p = NULL;
	}
	else if (oldp == NULL)
	{
		p = mem_malloc_debug(n, fil, lin, s);
	}
	else
	{
		p = mem_malloc_debug(n, fil, lin, s);
		if (p != NULL)
		{
			dl = mem_ptrtodl(oldp);
			if (dl->nbytes < n)
			n = dl->nbytes;
			memcpy(p,oldp,n);
			mem_free_debug(oldp,fil,lin);
		}
	}
	return p;
}

/***************************/

void mem_check()
{
	register struct mem_debug *dl;

	for (dl = mem_alloclist.next; dl != NULL; dl = dl->next)
	{
		mem_checkptr(mem_dltoptr(dl));
	}
}


void mem_checkfree()
{
	if (pMono)
	{
		pMono->Out("\nchecking free..");
	}
	mem_malloc(10*1000000);
	if (pMono)
	{
		pMono->Out(" is ok");
	}
}

/***************************/

void mem_checkptr(void* p)
{	
	register struct mem_debug *dl;

	for (dl = mem_alloclist.next; dl != NULL; dl = dl->next)
	{
		if (p >= (void *) &(dl->data[0]) &&
			p < (void *)((char *)dl + sizeof(struct mem_debug)-1 + dl->nbytes))
		{
			goto L1;
		}
	}
	ASSERT(0);

L1:
	dl = mem_ptrtodl(p);
	if (dl->beforeval != BEFOREVAL)
	{
		PRINT "Pointer x%lx underrun\n",(long)p);
		goto err2;
	}
	if (*(long *) &dl->data[dl->nbytes] != AFTERVAL)
	{
		PRINT "Pointer x%lx overrun\n",(long)p);
		goto err2;
	}
	return;

err2:
	// back to text mode
	AShutDownVideo();
	// print error data
	mem_printdl(dl);
	ASSERT(0);
}

#else



/***************************/

void *mem_malloc(uint numbytes)
{
	void *p;

	if (numbytes == 0) 
	{
		return NULL;
	}

	while (1)
	{
		p = malloc(numbytes);
		if (p == NULL)
		{	
			if (mem_exception())
			{
				continue;
			}
		}
		#if !MEM_NOMEMCOUNT
		else
		{
			mem_count++;
		}
		#endif
		break;
	}

	// printf("malloc(%d) = x%lx, mem_count = %d\n",numbytes,p,mem_count);
	return p;
}




/***************************/

void *mem_calloc(uint numbytes)
{
	void *p;

	if (numbytes == 0)
	{
		return NULL;
	}

	while (1)
	{
		p = calloc(numbytes,1);
		if (p == NULL)
		{
			if (mem_exception())
			{
				continue;
			}
		}
		#if !MEM_NOMEMCOUNT
		else
		{
			mem_count++;
		}
		#endif
		break;
	}

	// printf("calloc(%d) = x%lx, mem_count = %d\n",numbytes,p,mem_count);
	return p;
}




/***************************/

void *mem_realloc(void* oldmem_ptr, uint newnumbytes)
{
	void *p;

	if (oldmem_ptr == NULL)
	{
		p = mem_malloc(newnumbytes);
	}
	else if (newnumbytes == 0)
	{
		mem_free(oldmem_ptr);
		p = NULL;
	}
	else
	{
		do
		{
			p = realloc(oldmem_ptr,newnumbytes);
		} while (p == NULL && mem_exception());
	}
	
	// printf("realloc(x%lx,%d) = x%lx, mem_count = %d\n",
	//				oldmem_ptr, newnumbytes, p, mem_count);
	return p;
}




/***************************/

void mem_free(void* ptr)
{
	// printf("free(x%lx) mem_count=%d\n", ptr, mem_count);

	if (ptr != NULL)
	{
		#if !MEM_NOMEMCOUNT
			ASSERT(mem_count != 0);
			mem_count--;
		#endif
		free(ptr);
	}
}



/***************************/
/* This is our low-rent fast storage allocator	*/

static void *heap;
static size_t heapleft;


/***************************/

void *mem_fmalloc(uint numbytes)
{	
	void *p;

	printf ("mem_fmalloc()\n");

	if (sizeof(size_t) == 2)
	{
		numbytes = (numbytes + 1) & ~1;		// word align
	}
	else
	{
		numbytes = (numbytes + 3) & ~3;		// dword align
	}

	// This ugly flow-of-control is so that the most common case
	//	drops straight through.

	if (!numbytes)
	{
		return NULL;
	}

	if (numbytes <= heapleft)
	{
L2:
		p = heap;
		(char *)heap += numbytes;
		heapleft -= numbytes;
		return p;
	}

	heapleft = (numbytes + 4095) & ~4095;	// round to next 4096 boundary
L1:
	heap = malloc(heapleft);
	
	if (!heap)
	{
		if (mem_exception())
		{
			goto L1;
		}
		return NULL;
	}
	goto L2;
}


/***************************/

void *mem_fcalloc(uint numbytes)
{
	void *p;

	printf ("mem_fcalloc()\n");
	
	p = mem_fmalloc(numbytes);
	
	return p ? memset(p, 0, numbytes) : p;
}


/***************************/

char *mem_fstrdup(const char *s)
{
	char *p;

	p = s ? (char *) mem_fmalloc((unsigned) strlen(s) + 1) : NULL;
	return p ? strcpy(p, s) : p;
}

#endif




/***************************/

void mem_init()
{
	if (mem_inited == 0)
	{	mem_count = 0;
		mem_scount = 0;
		fp = NULL;
		mem_behavior = MEM_RETNULL;

		#if MEM_DEBUG
			mem_numalloc = 0;
			mem_maxalloc = 0;
			mem_alloclist.next = NULL;
		#endif
		
		#if __ZTC__
			// Necessary if mem_sfree() calls free() before any
			// calls to malloc().
			free(malloc(1));	// initialize storage allocator
		#endif
	}
	mem_inited++;
}

/***************************/

// set to false if crashing
bool	fPrintUnfreedPtrs = TRUE;

void mem_term()
{
	if (mem_inited == 1)
	{
		#if MEM_DEBUG
			register struct mem_debug *dl;

			if (fPrintUnfreedPtrs)
			{
				for (dl = mem_alloclist.next; dl; dl = dl->next)
				{
					PRINT "Unfreed pointer: ");
					mem_printdl(dl);
				}
			}
			#if 0
				PRINT "Max amount ever allocated == %ld bytes\n", mem_maxalloc);
			#endif
		#else
			if (mem_count)
			{
				PRINT "%d unfreed items\n",mem_count);
			}
			if (mem_scount)
			{
				PRINT "%d unfreed s items\n",mem_scount);
			}
		#endif // MEM_DEBUG

		//jc ASSERT(mem_count == 0 && mem_scount == 0);
	}
	mem_inited--;

	#ifdef MEM_DEBUG
		ASSERT(mem_inited >= 0);
	#endif
}


//void mem_dump()
//{
//	FILE*	pFile;
//	register struct mem_debug *dl;
//
//	if (mem_inited == 1)
//	{
//		#if MEM_DEBUG
//
//			if (AResMgr())
//			{
//				APurge(0xffffffff);
//			}
//
//			pFile = fopen("mem.dmp", "at");
//			fprintf(pFile, "\n\n ****** Mem dump ****** \n\n");
//			fprintf(pFile, "File           line   bytes      ptr        ID              clocks\n");
//			fprintf(pFile, "------------   ----   --------   --------   --------------- -------\n");
//
//			for (dl = mem_alloclist.next; dl; dl = dl->next)
//			{	
//				fprintf (pFile, "%-12s   %4d   %8d   %p   %s         %d\n",
//					dl->file,
//					dl->line,
//					dl->nbytes,
//					(long)mem_dltoptr(dl),
//					dl->str,
//					dl->clocks);
//			}
//			fclose (pFile);
//		#else
//			if (mem_count)
//				PRINT "%d unfreed items\n",mem_count);
//			if (mem_scount)
//				PRINT "%d unfreed s items\n",mem_scount);
//		#endif /* MEM_DEBUG */
//	}
//}


#undef next
#undef prev
#undef file
#undef line
#undef nbytes
#undef beforeval

