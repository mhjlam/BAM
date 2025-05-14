#ifndef NASI_HPP
#define NASI_HPP

#define NASI_CTS_STATUS     0x20
#define NASI_DSR_STATUS     0x10
#define NASI_DCD_STATUS     0x08
#define NASI_BREAK_STATUS   0x80

#if !defined( GCPP_OS2 ) && !defined( GCPP_WIN32 ) && !defined( GCPP_WIN32S )

class GFNasi : public GFComLink
{
  friend class GCPP_CLASS_TYPE GFSerial;
  friend class GCPP_CLASS_TYPE GFDataFormat;
  friend class GCPP_CLASS_TYPE GFLineStatus;

  public:
    GCPP_PROTO GFNasi( char GCPP_DLL_FAR *name );
    GCPP_PROTO GFNasi( GFNasi GCPP_DLL_FAR &prev );
    GCPP_PROTO ~GFNasi( void );

  protected:
    void GCPP_PROTO Initialize( int com );
    int GCPP_PROTO Open( void );
    int GCPP_PROTO Close( void );
    void GCPP_PROTO GetBufferInfo( GFLinkInfo GCPP_DLL_FAR &sbc,
      int status_flag = GCPP_RXINFO | GCPP_TXINFO | GCPP_FLOWINFO );
    int GCPP_PROTO NewLinkSize( unsigned insize,
                                unsigned inroom,
                                unsigned outsize = 0 );
    int GCPP_PROTO SetHandShake( int inhand, int outhand );
    int GCPP_PROTO GetHandShake( int GCPP_DLL_FAR &inhand,
                                int GCPP_DLL_FAR &outhand );
    int GCPP_PROTO LineBreak( int state );
    int GCPP_PROTO LineControl( int line, int state = -1 );
    int GCPP_PROTO SetDataFormat( long bd, char pr, int dl, int st );
    void GCPP_PROTO GetDataFormat( long GCPP_DLL_FAR &bd,
                                    char GCPP_DLL_FAR &pr,
                                    int GCPP_DLL_FAR &dl,
                                    int GCPP_DLL_FAR &st );
    void GCPP_PROTO UpdateLineStatus( GFlinestat GCPP_DLL_FAR &line );
    int GCPP_PROTO PutChar( unsigned char b );
    unsigned GCPP_PROTO PutString( const unsigned char GCPP_DLL_FAR *string );
    unsigned GCPP_PROTO PutBuffer( const unsigned char GCPP_DLL_FAR *buf,
                                    unsigned count );
    int GCPP_PROTO PutImm( unsigned char b );
    int GCPP_PROTO GetChar( void );
    unsigned GCPP_PROTO GetBuffer( unsigned char GCPP_DLL_FAR *buffer,
                                    unsigned count );
    int GCPP_PROTO ClearLink( int dir, unsigned numbytes = 0 );
    void GCPP_PROTO Flush( void );
    int GCPP_PROTO SystemIdle( void );

    int GCPP_PROTO NasiInit( void );
    int GCPP_PROTO SetDTRNasi( int control );
    int GCPP_PROTO SetRTSNasi( int control );
    void GCPP_PROTO GetNames( void );
    int GCPP_PROTO AttachNasi( void );
    void GCPP_PROTO NasiParsePortName( char GCPP_DLL_FAR *name );

    void GCPP_PROTO ReportLinkStatus( VIEWER_OUTPUT viewer_output );
    void GCPP_PROTO DumpNasiItems( VIEWER_OUTPUT viewer_output );
    void GCPP_PROTO DumpAllVcStatus( VIEWER_OUTPUT viewer_output );
    void GCPP_PROTO DumpConfig( VIEWER_OUTPUT viewer_output );
    void GCPP_PROTO DumpStatus( VIEWER_OUTPUT viewer_output );

    char server_name[ 9 ];
    char general_port_name[ 9 ];
    char specific_port_name[ 15 ];
    char service_name[ 15 ];
    char session_name[ 17 ];
    short int vc;
    short int line_status;
    char pconfig[ 15 ];
    int inmode;
    int outmode;
    int NASI_RTS;
    int NASI_DTR;
    unsigned short NASI_ModemStatus;
    int InUseCounter;
};

typedef struct {
    short int vc;
    char buf[ 34 ];
    char Server[ 9 ];
    char GeneralPort[ 9 ];
    char SpecificPort[ 15 ];
    int NcsPortNumber;
    int PortStatus;
} NASI_QUERY;

int GCPP_FUN NasiGetSession( char GCPP_DLL_FAR *session );
int GCPP_FUN NasiSetSession( char GCPP_DLL_FAR *name,
                            char GCPP_DLL_FAR *password,
                            char GCPP_DLL_FAR *session_name,
                            int global );

int GCPP_FUN NasiLoaded( void );
int GCPP_FUN NasiQueryFirstPort( NASI_QUERY GCPP_DLL_FAR *q );
int GCPP_FUN NasiQueryNextPort( NASI_QUERY GCPP_DLL_FAR *q );
void GCPP_FUN NasiCloseQuery( NASI_QUERY GCPP_DLL_FAR *q );

int _NasiAllocateVc( short int GCPP_DLL_FAR *pvc );
int _NasiDisconnectVc( unsigned short int vc );

#if defined( __cplusplus )
extern "C" {
#endif

#if defined( GCPP_WINDOWS )
typedef WORD (FAR PASCAL * SET_SESSION_NAME)(char far *);
typedef WORD (FAR PASCAL * ALLOCATE_VC)(short int far *pvc);
typedef WORD (FAR PASCAL * QUERY_NAME_SERVICE)(short int vc,
                                                       short int Length,
                                                       char far *pBuffer,
                                                       char QueryType);
typedef WORD (FAR PASCAL * DISCONNECT)(short int vc);
typedef WORD (FAR PASCAL * INITIALIZE)(short int vc,
                                               char far *pConfig,
                                               char far *pPort );
typedef WORD (FAR PASCAL * SET_SERVICE_NAME)(unsigned short vc,
                                                     char far *pServiceName);
typedef WORD (FAR PASCAL *FLUSH_RX_BUFFER)(unsigned short int vc);
typedef WORD (FAR PASCAL *FLUSH_TX_BUFFER)(unsigned short int vc);
typedef WORD (FAR PASCAL *GET_TX_STATUS)(unsigned short int vc,
                                                 char far *pXStatus,
                                                 char far *pBoardStat);
typedef WORD (FAR PASCAL *GET_RX_STATUS)(unsigned short int vc,
                                                 char far *pXStatus);
typedef WORD (FAR PASCAL *GET_EXTERNAL_STATUS)(unsigned short int vc,
                                                       char far *pStatus);
typedef WORD (FAR PASCAL *READ_BLOCK)(unsigned short int vc,
                                              short unsigned int far *pLength,
                                              char far *pBuffer);
typedef WORD (FAR PASCAL *WRITE_BLOCK)(unsigned short int vc,
                                               unsigned short int far *pLength,
                                               char far *pBuffer);
typedef WORD (FAR PASCAL *CONTROL_REQUEST)(unsigned short int vc,
                                                   char RequestType,
                                                   char far *pBuffer);
typedef WORD (FAR PASCAL *GET_SERVER_NAME)(unsigned short int vc,
                                                   char far *pServerName);
typedef WORD (FAR PASCAL *GET_GENERAL_NAME)(unsigned short int vc,
                                                    char far *pGeneralName);
typedef WORD (FAR PASCAL *GET_SERVICE_NAME)(unsigned short int vc,
                                                    char far *pServiceName);
typedef WORD (FAR PASCAL *GET_SPECIFIC_NAME)(unsigned short int vc,
                                                     char far *pServiceName);
typedef WORD (FAR PASCAL *GET_SESSION_NAME)(char far *pSessionName);
typedef WORD (FAR PASCAL *GET_STATUS)(unsigned short int vc,
                                              unsigned short int far *pStatus );

extern SET_SESSION_NAME _nasi_set_session_name;
extern ALLOCATE_VC _nasi_allocate_vc;
extern QUERY_NAME_SERVICE _nasi_query_name_service;
extern DISCONNECT _nasi_disconnect_vc;
extern INITIALIZE _nasi_initialize;
extern SET_SERVICE_NAME _nasi_set_service_name;
extern FLUSH_RX_BUFFER _nasi_flush_rx_buffer;
extern FLUSH_TX_BUFFER _nasi_flush_tx_buffer;
extern GET_TX_STATUS _nasi_get_tx_status;
extern GET_RX_STATUS _nasi_get_rx_status;
extern GET_EXTERNAL_STATUS _nasi_get_external_status;
extern READ_BLOCK _nasi_read;
extern WRITE_BLOCK _nasi_write;
extern CONTROL_REQUEST _nasi_control_request;
extern GET_SERVER_NAME _nasi_get_server_name;
extern GET_GENERAL_NAME _nasi_get_general_name;
extern GET_SERVICE_NAME _nasi_get_service_name;
extern GET_SPECIFIC_NAME _nasi_get_specific_name;
extern GET_SESSION_NAME _nasi_get_session_name;
extern GET_STATUS _nasi_get_status;

#endif

#if defined( __cplusplus )
};
#endif

#endif
#endif

