/*
Copyright (C) Magna Carta Software, Inc. 1988-1995. All Rights Reserved.
C COMMUNICATIONS TOOLKIT
CASTYPES.H -- Some Intel/DCA CAS type definitions.
*/

#if !defined(CASTYPES_H_INCLUDED)
    #define CASTYPES_H_INCLUDED

    #if defined(__cplusplus)
        extern "C" {
    #endif

	typedef struct cas_ftr {
		char            ftype;          /* FAX type */
		char            text_size;      /* text size for ASCII files */
		char            status;         /* what has been done to this file? */
		unsigned long   bytes_sent;     /* bytes_sent so far */
		unsigned long   file_bytes;     /* file size */
		unsigned short  pages_sent;     /* # of pages sent so far */
		unsigned short  file_pages;     /* file size (pages) */
		char            fpath[80];      /* file path */
		char            increments;     /* 1/8 inch. increments to page length */
		char            pl;             /* page length */
		char            reserved1[31];  /* RESERVED */
	} CAS_FILE_TRANSFER_RECORD;

	typedef struct cas_ftrx {
		struct cas_ftrx  DLL_FAR_ *next; /* next FTR in linked list */
		CAS_FILE_TRANSFER_RECORD cftr;
	} CAS_FILE_TRANSFER_RECORDX;


    typedef struct cas_ftr_abbrev {
		char            ftype;          /* FAX type (0=ASCII, 1=PCX, 2=DCX) */
        char            text_size;      /* text size for ASCII files */
        unsigned short  event_time;     /* DOS file time format */
        unsigned short  event_date;     /* DOS file time format */
        char            dest[32];       /* "To:" field (ASCIIZ) */
        char            fpath[80];      /* file path */
        char            phone_num[47];  /* phone number to call (ASCIIZ) */
        char            app_tag[64];    /* app.-specific tag string */
        char            reserved1;      /* reserved -- initialized to zero */
        char            cover_flag;     /* send cover? */
        char            reserved2[23];  /* reserved -- initialized to zero */
        char            *cover_text;    /* cover page text */
	} CAS_CONTROL_FILE_ABBREVIATED;


    typedef struct cas_control_file {
        char            event_type;     /* send, receive, etc. */
        char            transfer_type;  /* 200x200, 100x100 file, etc. */
        short           event_status;   /* waiting, completed, etc. */
        unsigned short  event_time;     /* DOS file time format */
        unsigned short  event_date;     /* DOS file time format */
        short           num_files;      /* number of files to transfer */
        unsigned short  ftr_offset;     /* offset in ctl file of file transfer record */
        char            phone_num[47];  /* phone number to call (ASCIIZ) */
        char            app_tag[64];    /* app.-specific tag string */
        char            reserved1;      /* reserved -- initialized to zero */
        char            connect_secs;   /* length of phone connect (secs) */
        char            connect_mins;   /* length of phone connect (mins) */
        char            connect_hours;  /* length of phone connect (hours) */
        unsigned long   pages;          /* # pages in all files */
        unsigned long   pages_sent;     /* # pages sent thus far */
        unsigned short  files_sent;     /* # files sent thus far */
        char            cover_flag;     /* send cover? */
        unsigned short  errors;         /* # of TX errors for this ctl. file */
        char            fDeleteFiles;   /* flag: delete files (1.2) */
        unsigned short  hParent;        /* handle to parent event (1.2) */
        char            reserved2[53];  /* reserved -- initialized to zero */
        char            cas_reserved[20];/* reserved for CAS impl. (1.2) */
        char            fCoverRead;     /* flag: cover read by user? (1.2) */
        char            fNoPageHdr;     /* flag: suppress page headers ((1.2) */
        char            csid[21];       /* remote CSID (initialized by hardware) */
        char            dest[32];       /* "To:" field (ASCIIZ) */
        char            sender[32];     /* "From:" field (ASCIIZ) */
        char            lpath[80];      /* Name of PCX logo file */
    } CAS_CONTROL_FILE;

    #if defined(__cplusplus)
        }
    #endif
#endif
