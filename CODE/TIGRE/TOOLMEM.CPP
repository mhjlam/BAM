//
// TOOLMEM.CPP
//
// May 26, 1989
// WATCOM: September 20, 1994  BKH
// (C) 1986-1991 by Walter Bright  All Rights Reserved.
// Modified December 7, 1993 by Tachyon, Inc.
//
// Limited memory management routines for TIGRE tools.
//
//----[]-------------------------------------------------------------


#include	<assert.h>
#include	<io.h>
#include	<new.h>
#include	<stdarg.h>
#include	<stdio.h>
#include	<stdlib.h>
#include <string.h>

#include "os.hpp"
#include "toolmem.hpp"

#ifndef MEM_NOMEMCOUNT
	#define MEM_NOMEMCOUNT	0
#endif


//----[Stub Functions]-----------------------------------------------

// This is here for convience. Most tools seem to need this.

// If we include api to get APanic, then we have to include graphmgr
// and many other objs.  Let's just make a stub function to catch
// all calls to APanic.

void
_Panic(char* msg, char* fileName, int lineNum)
{
	char	err[256];

	sprintf (err, "\n%s\n\tat line %d, file %s\n", msg, lineNum, fileName);
	printf(err);
	exit(1);
}


int mem_inited = 0;		/* != 0 if initialized			*/

static int mem_behavior = MEM_RETNULL;
static int (*fp) P((void)) = NULL;	/* out-of-memory handler	*/
static int mem_count;		/* # of allocs that haven't been free'd	*/
static int mem_scount;		/* # of sallocs that haven't been free'd */

#define PRINT	printf(	/* stderr can't be redirected with MS-DOS	*/



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
		assert(0 <= flag && flag <= MEM_RETRY);
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
//int line;
//const char *file;
//const char *s;
{
	char *p;

	p = s
		 ? (char *) mem_malloc_debug((unsigned) strlen(s) + 1,file,line)
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

#include "api.hpp"

#undef new
void * operator new(size_t size)
{
	void*	p;

	if (!mem_inited)
	{
		mem_init();
	}

	while (1)
	{	
		if (size == 0) {
			size++;
		}

		assert (mem_inited);

		p = mem_malloc(size);

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
	mem_free(p);
}



}
#endif



#ifdef MEM_DEBUG

static long mem_maxalloc;	/* max # of bytes allocated		*/
static long mem_numalloc;	/* current # of bytes allocated		*/

#define BEFOREVAL	0x4F464542L	// value to detect underrun
#define AFTERVAL	0x45544641L	// value to detect overrun


/* The following should be selected to give maximum probability that	*/
/* pointers loaded with these values will cause an obvious crash. On	*/
/* Unix machines, a large value will cause a segment fault.		*/
/* MALLOCVAL is the value to set malloc'd data to.			*/

#if MSDOS || __OS2__ || __NT__
#define BADVAL		0xFF
#define MALLOCVAL	0xEE
#else
#define BADVAL		0x7A
#define MALLOCVAL	0xEE
#endif

/* Disable mapping macros	*/
#undef	mem_malloc
#undef	mem_calloc
#undef	mem_realloc
#undef	mem_free

/* Create a list of all alloc'ed pointers, retaining info about where	*/
/* each alloc came from. This is a real memory and speed hog, but who	*/
/* cares when you've got obscure pointer bugs.				*/

struct mem_debug;

struct mh
{
	struct mem_debug	*Mnext;		// next in list
	struct mem_debug	*Mprev;		// previous value in list
	const char 			*Mfile;		// filename of where allocated
	int 					Mline;		// line number of where allocated
	unsigned 			Mnbytes;		// size of the allocation
	long 					Mbeforeval;	// detect underrun of data
};

static struct mem_debug
{
	struct mh m;
	char data[1];		/* the data actually allocated		*/
} mem_alloclist =
{
	{
		(struct mem_debug *) NULL,
		(struct mem_debug *) NULL,
		NULL,
		11111,
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
	PRINT "alloc'd from file '%s' line %d nbytes %d ptr %p\n",
		dl->file,dl->line,dl->nbytes,(long)mem_dltoptr(dl));
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

void *mem_calloc(unsigned u)
{
		//return mem_calloc_debug(u,__FILE__,__LINE__);
		return mem_calloc_debug(u,szFileTrack, lineTrack);
}

void *mem_malloc(unsigned u)
{
	  	//return mem_malloc_debug(u,__FILE__,__LINE__);
	  	return mem_malloc_debug(u,szFileTrack,lineTrack);
}

void *mem_realloc(void* p, unsigned u)
{
	  	//return mem_realloc_debug(p,u,__FILE__,__LINE__);
	  	return mem_realloc_debug(p,u,szFileTrack, lineTrack);
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

void *mem_malloc_debug(unsigned n, const char* fil, int lin)
{
	void *p;

	fil = fil;		// Gets rid of unreferenced warning
	lin = lin;

	p = mem_calloc_debug(n, szFileTrack, lineTrack);
	return p;
}

void *mem_calloc_debug(unsigned n, const char* fil, int lin)
{
	struct mem_debug *dl;

	do
	{
		dl = (struct mem_debug *)
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
	{	PRINT "More frees than allocs at ");
		goto err;
	}

	dl = mem_ptrtodl(ptr);
	if (dl->beforeval != BEFOREVAL)
	{
		PRINT "Pointer x%lx underrun\n",(long)ptr);
		PRINT "'%s'(%d)\n",fil,lin);
		goto err2;
	}

	if (*(long *) &dl->data[dl->nbytes] != AFTERVAL)
	{
		PRINT "Pointer x%lx overrun\n",(long)ptr);
		goto err2;
	}
	mem_numalloc -= dl->nbytes;
	if (mem_numalloc < 0)
	{	PRINT "error: mem_numalloc = %ld, dl->nbytes = %d\n",
			mem_numalloc,dl->nbytes);
		goto err2;
	}

	/* Remove dl from linked list	*/
	if (dl->prev)
		dl->prev->next = dl->next;
	if (dl->next)
		dl->next->prev = dl->prev;

	/* Stomp on the freed storage to help detect references	*/
	/* after the storage was freed.				*/
	memset((void *) dl,BADVAL,sizeof(*dl) + dl->nbytes);
	mem_count--;

	free((void *) dl);
	return;

err2:
	mem_printdl(dl);
err:
	PRINT "free'd from ");
	mem_fillin(fil,lin);
	assert(0);
	/* NOTREACHED */
}



/*******************
 * Debug version of mem_realloc().
 */

void *mem_realloc_debug(void* oldp, unsigned n, const char* fil, int lin)
{	void *p;
	struct mem_debug *dl;

	if (n == 0)
	{	mem_free_debug(oldp,fil,lin);
	p = NULL;
	}
	else if (oldp == NULL)
	p = mem_malloc_debug(n,fil,lin);
	else
	{
	p = mem_malloc_debug(n,fil,lin);
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
{	register struct mem_debug *dl;

	for (dl = mem_alloclist.next; dl != NULL; dl = dl->next)
	mem_checkptr(mem_dltoptr(dl));
}

/***************************/

void mem_checkptr(register void* p)
{	register struct mem_debug *dl;

	for (dl = mem_alloclist.next; dl != NULL; dl = dl->next)
	{
	if (p >= (void *) &(dl->data[0]) &&
		p < (void *)((char *)dl + sizeof(struct mem_debug)-1 + dl->nbytes))
		goto L1;
	}
	assert(0);

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
	mem_printdl(dl);
	assert(0);
}

#else



/***************************/

void *mem_malloc(unsigned numbytes)
{	void *p;

	if (numbytes == 0) 
	{
		return NULL;
	}

	while (1)
	{
		p = malloc(numbytes);
		if (p == NULL)
		{	
			if (mem_exception()) {
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

	/*printf("malloc(%d) = x%lx, mem_count = %d\n",numbytes,p,mem_count);*/
	return p;
}




/***************************/

void *mem_calloc(unsigned numbytes)
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
		{	if (mem_exception())
				continue;
		}
		#if !MEM_NOMEMCOUNT
		else
			mem_count++;
		#endif
		break;
	}

	/*printf("calloc(%d) = x%lx, mem_count = %d\n",numbytes,p,mem_count);*/
	return p;
}




/***************************/

void *mem_realloc(void* oldmem_ptr, unsigned newnumbytes)
{	void *p;

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
	
	/*printf("realloc(x%lx,%d) = x%lx, mem_count = %d\n",oldmem_ptr,newnumbytes,p,mem_count);*/
	return p;
}




/***************************/

void mem_free(void* ptr)
{
	/*printf("free(x%lx) mem_count=%d\n",ptr,mem_count);*/
	if (ptr != NULL)
	{
		#if !MEM_NOMEMCOUNT
		assert(mem_count != 0);
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

void *mem_fmalloc(unsigned numbytes)
{	
	void *p;

	printf ("mem_fmalloc()\n");

	if (sizeof(size_t) == 2)
	{
		numbytes = (numbytes + 1) & ~1;		/* word align	*/
	} else {
		numbytes = (numbytes + 3) & ~3;		/* dword align	*/
	}

	/* This ugly flow-of-control is so that the most common case
		drops straight through.
	*/

	if (!numbytes) {
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

	heapleft = (numbytes + 4095) & ~4095;	/* round to next 4096 boundary */
L1:
	heap = malloc(heapleft);
	
	if (!heap)
	{
		if (mem_exception()) {
			goto L1;
		}
		return NULL;
	}
	goto L2;
}


/***************************/

void *mem_fcalloc(unsigned numbytes)
{
	void *p;

	printf ("mem_fcalloc()\n");
	
	p = mem_fmalloc(numbytes);
	
	return p ? memset(p,0,numbytes) : p;
}


/***************************/

char *mem_fstrdup(const char *s)
{
	char *p;

	p = s ? (char *) mem_fmalloc((unsigned) strlen(s) + 1) : NULL;
	return p ? strcpy(p,s) : p;
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
			/* Necessary if mem_sfree() calls free() before any	*/
			/* calls to malloc().					*/
			free(malloc(1));	/* initialize storage allocator	*/
		#endif
	}
	mem_inited++;
}

/***************************/

bool	hey_were_crashing_anyway_lighten_up = FALSE;

void mem_term()
{
	if (mem_inited == 1)
	{
		#if MEM_DEBUG
			register struct mem_debug *dl;

			if (hey_were_crashing_anyway_lighten_up)
			{
				for (dl = mem_alloclist.next; dl; dl = dl->next)
				{	PRINT "Unfreed pointer: ");
					mem_printdl(dl);
				}
			}
			#if 0
				PRINT "Max amount ever allocated == %ld bytes\n", mem_maxalloc);
			#endif
		#else
			if (mem_count)
				PRINT "%d unfreed items\n",mem_count);
			if (mem_scount)
				PRINT "%d unfreed s items\n",mem_scount);
		#endif /* MEM_DEBUG */

		//jc assert(mem_count == 0 && mem_scount == 0);
	}
	mem_inited--;

	#ifdef MEM_DEBUG
		assert(mem_inited >= 0);
	#endif
}

#undef next
#undef prev
#undef file
#undef line
#undef nbytes
#undef beforeval
