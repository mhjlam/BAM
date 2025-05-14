/*
Copyright (C) Magna Carta Software, Inc. 1988-1995.  All Rights Reserved
C COMMUNICATIONS TOOLKIT
CAS.H -- Intel/DCA CAS prototypes and definitions.
*/

#if !defined(CAS_H_INCLUDED)
    #define CAS_H_INCLUDED

    #if defined(__cplusplus)
        extern "C" {
    #endif

    #if !defined(COMPAT_H_INCLUDED)
        #include <compat.h>
    #endif

    #if !defined(CCT_DEVELOPMENT)
        typedef void DLL_FAR_ * hCAS;
        #define CAS hCAS

        /* PRAGMAS TO ALIGN STRUCTURES ON BYTE BOUNDARIES */
        #if defined(__BORLANDC__)
            #if (__BORLANDC__ >= 0X400)
                #pragma option -a-
            #endif
        #endif
        #if defined(__HIGHC__)
            #pragma Off (Align_members)
        #endif
        #if defined(_INTELC32_)
            #pragma noalign(cas_control_file)
            #pragma noalign(cas_external_data_block)
            #pragma noalign(cas_ftr)
        #endif
        #if defined(MSC)
            #pragma pack(1)
            #if defined(_MSC_VER)
                #pragma optimize("e", off)
            #endif
        #endif
        #if defined(__WATCOMC__)
            #pragma pack(1)
        #endif
    #endif


    /* CONTROL FILE FIELD VALUES */
    #define CAS_SEND                    0       /* VALUES FOR "CAS_EVENT_TYPE" */
    #define CAS_RECEIVE                 1
    #define CAS_POLLED_SEND             2
    #define CAS_POLLED_RECEIVE          3
    #define CAS_GROUP_SEND              4           /* v1.2 */
    #define CAS_GROUP_POLLED_RECEIVE    5           /* v1.2 */

    #define CAS_200_DPI         0               /* VALUES FOR "CAS_TRANSFER_TYPE" */
    #define CAS_100_DPI         1
    #define CAS_FILE            2

    /* FILE TRANSFER RECORD FIELD VALUES */
    #define CAS_ASCII           0               /* VALUES FOR "CAS_FILE_TYPE" */
    #define CAS_PCX             1
    #define CAS_DCX             2
    #define CAS_HP_PCL4         16                  /* v1.2 */
    #define CAS_EPSON_FX85      17                  /* v1.2 */

    #define CAS_80_COL          0               /* VALUES FOR "CAS_TEXT_SIZE" */
    #define CAS_132_COL         1

    /* EVENT STATUS VALUES (used in cas_findfirst(), offset 2 in control file */
    #define CAS_COMPLETED       0
    #define CAS_WAITING         1
    #define CAS_DIALLED         2
    #define CAS_SENDING         3
    #define CAS_RECEIVING       4
    #define CAS_ABORTED         5
    #define CAS_ANY             (-1)

    /* OPEN A FILE QUEUE VALUES */
    #define CAS_TASK_QUEUE              0
    #define CAS_RECEIVE_QUEUE           1
    #define CAS_LOG                     2
    #define CAS_SEND_EVENTS             3           /* v1.2 */
    #define CAS_RECEIVE_EVENTS          4           /* v1.2 */

    /* DELETE FILES FLAG (CAS v1.2) */
    #define CAS_DELETE_FILES    100
    #define CAS_NODELETE        0
    #define CAS_DEL_SUCCESS     1
    #define CAS_DEL_ALWAYS      2

    /* CAS ERRORS */
	#define CASERR_NOMOREEVENTS         (-516)
	#define CASERR_INVALIDQUEUETYPE     (-519)
	#define CASERR_COMMBOARDBUSY        (-521)
	#define CASERR_FILEEXISTS           (-524)
	#define CASERR_BADPCXFILEHEADER     (-642)
	#define CASERR_UNEXPECTEDEOF        (-643)
	#define CASERR_FILENOTFOUND         (-770)
	#define CASERR_PATHNOTFOUND         (-771)
	#define CASERR_WRITEFAULT           (-797)


    struct cct_date {
        unsigned short    year;
        unsigned short    month;
        unsigned short    day;
        unsigned short    hour;
        unsigned short    minute;
        unsigned short    second;
    };

    #include <castypes.h>

    typedef struct cas_external_data_block {
        char            ver_major;          /* CAS major version number */
        char            ver_minor;          /* CAS minor version number */
        char            path_rm[68];        /* path to resident mgr. */
        char            phonebook_name[13]; /* name of default phonebook */
        char            logofile_name[13];  /* name of default logo file */
        char            sender_name[32];    /* name of default sender */
        char            csid[21];           /* CSID of fax device */
        char            reserved[107];      /* reserved */
    } CAS_EXTERNAL_DATA_BLOCK;

    typedef struct cas_current_event_status {
        CAS_CONTROL_FILE        cf;
        CAS_FILE_TRANSFER_RECORD ftr;
    } CAS_CURRENT_EVENT_STATUS;

    extern unsigned char cas_multiplex_num;      /* CAS multiplex number */

    short EXPORT_ cas_abort_current_event(void);
    short EXPORT_ cas_autoreceive_state(unsigned short func, unsigned short rings);
    short EXPORT_ cas_deinit(hCAS c);
    short EXPORT_ cas_findfirst(short status, short direction, unsigned short queue, unsigned short DLL_FAR_ *event_handle);
    short EXPORT_ cas_findnext(unsigned short queue, unsigned short DLL_FAR_ *event_handle);
    short EXPORT_ cas_free_all(hCAS c);
    short EXPORT_ cas_get_ces(CAS_CURRENT_EVENT_STATUS FAR_ *status, unsigned short DLL_FAR_ *event_num);
    short EXPORT_ cas_get_event_date(unsigned short event_handle, unsigned short queue, struct cct_date DLL_FAR_ *event_date);
    short EXPORT_ cas_get_event_time(unsigned short event_handle, unsigned short queue, struct cct_date DLL_FAR_ *event_date);
    short EXPORT_ cas_get_external_data_block(CAS_EXTERNAL_DATA_BLOCK FAR_ *edb);
    short EXPORT_ cas_get_hardware_status(char FAR_ *status);
    short EXPORT_ cas_get_queue_status(unsigned short queue, unsigned short DLL_FAR_ *num_ctl, unsigned short DLL_FAR_ *num_rx);
    short EXPORT_ cas_move_received_file(unsigned short event_handle, unsigned short num, char FAR_ *new_name);
    short EXPORT_ cas_open(unsigned short event_handle, unsigned short file_num, unsigned short queue, unsigned short DLL_FAR_ *file_handle);
    short EXPORT_ cas_run_diagnostics(unsigned short mode);
    short EXPORT_ cas_set_multiplex_num(unsigned short num);
    short EXPORT_ cas_set_cover_page_status(unsigned short event_handle, unsigned short status);
    short EXPORT_ cas_set_task_date(unsigned short event_handle, struct cct_date DLL_FAR_ *event_date);
    short EXPORT_ cas_set_task_parameters(hCAS c, short parm, DWORD value);
    short EXPORT_ cas_set_task_time(unsigned short event_handle, struct cct_date DLL_FAR_ *event_date);
    short EXPORT_ cas_submit(char FAR_ *tcf);
    short EXPORT_ cas_submit_file(char FAR_ *status);
    short EXPORT_ cas_unlink(unsigned short event_handle, unsigned short file_num, unsigned short queue);
    short EXPORT_ cas_unlinkall(unsigned short queue);
    short EXPORT_ cas_unload_resident_mgr(void);
    short EXPORT_ cas_version(void);


    /* UTILITY FUNCTIONS */
    unsigned short  EXPORT_ todosdate(unsigned short year, unsigned short month, unsigned short day);
    unsigned short  EXPORT_ todostime(unsigned short hour, unsigned short minute, unsigned short sec);


    /* HIGH-LEVEL FUNCTIONS */
    hCAS  EXPORT_ cas_create_task(unsigned short etype, char DLL_FAR_ *from, char DLL_FAR_ *to, char DLL_FAR_ *num, unsigned short date, unsigned short time);
    short EXPORT_ cas_exist(void);
    short EXPORT_ cas_queue_file(hCAS c, unsigned short ftype, unsigned short text_size, char DLL_FAR_ *fpath, unsigned short increments, unsigned short pl);
    short EXPORT_ cas_send_one_file(char DLL_FAR_ *filename, char DLL_FAR_ *to, char DLL_FAR_ *phone_num);
    short EXPORT_ cas_submit_task(hCAS c, char DLL_FAR_ *fname, short mode, char DLL_FAR_ *lpath, char DLL_FAR_ *cover);

    #if !defined(EOF)
        #define EOF             (-1)
    #endif
    #define NO_RAM              (-102)

    /* COMPATIBILITY */
    #define isacas()        cas_exist()

    #if defined(__cplusplus)
        }
    #endif
#endif
