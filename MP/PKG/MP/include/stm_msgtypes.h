#ifndef __STM_MSGTYPES_H__
#define __STM_MSGTYPES_H__

#include "extconn_type.h"

//---------------------------------------------------------------------------
// 모든 통계 구조체는 아래와 같이 공통으로 사용함
//---------------------------------------------------------------------------
#define STAT_TYPE_START			  STAT_RADIUS_TYPE

#define STAT_RADIUS_TYPE			EXTCONN_TYPE_RADIUS
#define STAT_ROME_TYPE				EXTCONN_TYPE_ROME
#define STAT_NSTEP_TYPE				EXTCONN_TYPE_NSTEP
#define STAT_GTPP_TYPE				EXTCONN_TYPE_GTPP	
#define STAT_PORT_TYPE				4	// 3 --> 4 변경함 
#define STAT_DIAM_TYPE				EXTCONN_TYPE_DIAM

#define STAT_TYPE_END			  STAT_DIAM_TYPE
// 2017.3.09 STAT_PORT_TYPE 는 내부Port에 대한 Traffic 통계를 수집한다고 함.
// 2017.3.09 NSCP관련 통계 Index는 "STAT_NSCP_TYPE" 로 새로 정의한다. 
// 처음에 EXTCONN과 STAT TYPE에 대한 index를 대칭적으로 하려고 했던것으로 판단되고 EXTCONN과 STAT의 index는 별개로 정의하도록 한다.


#define MAX_STAT_TYPE_NUM			10             // 10개 통계 종류 
//#define MAX_STAT_ITEM_NUM           MAX_CONN_NUM	//2015.06.23. 50				
#define MAX_STAT_ITEM_NUM           100				//2016.09.23. 50 -> 100 
//#define MAX_STAT_ITEM_NUM           520            // 통계 ROW 개수 ex> 연동별 통계가 나오면 연동 개수 	
#define MAX_MULTIPROC_NUM		    10             // 멀티 블럭			//TODO
#define	MAX_STAT_FIELD_NUM		    100			   // 컬럼 개수		
//#define	MAX_OPCODE_TYPE_NUM         8              // Operation 개수	//TODO
#define	MAX_OPCODE_TYPE_NUM         10              // Operation 개수	//TODO

#ifndef COMM_MAX_NAME_LEN
#define COMM_MAX_NAME_LEN	16
#endif

#define			STAT_KEY_MAX 128

typedef struct stm_comm_stat {
	unsigned int    strkeyNum;                      // 통계 출력할 string 개수 ( 키값 ) 

	char            strkey1[STAT_KEY_MAX];		    // Ex : HOST   ( 16 -> 128 )
	char            strkey2[COMM_MAX_NAME_LEN];		// 추가 Key 
	char            strkey3[COMM_MAX_NAME_LEN];		// 추가 key
	char            strkey4[COMM_MAX_NAME_LEN];		// 추가 Key
	char            strkey5[COMM_MAX_NAME_LEN];		// 추가 Key
	unsigned int    intNum1;                        // 통계 출력할 integer 개수 : Messag Type or Opcode 개수 ( ex> DTR/DTA .. DRR/DRA ) 
	unsigned int    intNum2;                        // 통계 출력할 integer 개수 : 통계 컬럼 항목 개수  
													// ( ex> attempt success fail .... 의 종류 개수 ) 
	/* 20개의 OPCODE */
	// 메시지 타입(Opcode) string Value ( ex> DTR/DTA .. DRR/DRA ) 
	char			sdata[MAX_OPCODE_TYPE_NUM][COMM_MAX_NAME_LEN];	
	/* 20개의 OPCODE 에 대한 50개의 통계 칼럼. */
	// integer data, 첫번째 인자는 메시지 타입 index  
	 // unsigned int    idata[MAX_OPCODE_TYPE_NUM][MAX_STAT_FIELD_NUM];	
	unsigned long    ldata[MAX_OPCODE_TYPE_NUM][MAX_STAT_FIELD_NUM];	
} STM_CommonStatInfo;

typedef struct stm_comm_stat_shm {
	// MAX_STAT_TYPE_NUM : 통계 종류 ( ex> STAT_HSS_TYPE )    
	// MAX_MULTIPROC_NUM : 블럭 ID ( 멀티 블럭이 아닌경우 0 )  
	// MAX_STAT_ITEM_NUM : 연동포인트 ( HOST / DEST … ) 
	STM_CommonStatInfo		data[MAX_STAT_TYPE_NUM][MAX_MULTIPROC_NUM][MAX_STAT_ITEM_NUM];	// 현재 통계 값 
	STM_CommonStatInfo		prev[MAX_STAT_TYPE_NUM][MAX_MULTIPROC_NUM][MAX_STAT_ITEM_NUM];  // 이전 통계 값
	STM_CommonStatInfo		delta[MAX_STAT_TYPE_NUM][MAX_MULTIPROC_NUM][MAX_STAT_ITEM_NUM]; // Delta 통계 값 
} STM_CommonStatList;


//---------------------------------------------------------------------------
// MP와 OMP 통계 메시지 구조체 정의  
//---------------------------------------------------------------------------
#define MAX_STAT_DATA_NUM           25
#define STAT_LIMIT_CNT			    30

#pragma pack(1)
typedef struct {
	char            strkey1[STAT_KEY_MAX];		// Ex : ip addresss
	char            strkey2[STAT_KEY_MAX];		// Ex : ip addresss
	char            strkey3[COMM_MAX_NAME_LEN];		// 추가 key
	char            strkey4[COMM_MAX_NAME_LEN];		// 추가 Key
	char            strkey5[COMM_MAX_NAME_LEN];		// 추가 Key
	long    	    ldata[MAX_STAT_FIELD_NUM];					
} STM_CommonStatMsg;

typedef struct {
	unsigned int			num;
	STM_CommonStatMsg		info[MAX_STAT_DATA_NUM];
} STM_CommonStatMsgType;
#pragma pack(1)

/* STACK */
#define STM_IPSTR_LEN     64

/* add by hangkuk at 2020.05.07 */
#define MAX_STAT_HOSTNAME_LEN   80

/*************************************************/
/****    LOAD Statistic Structure           *****/
/*************************************************/
#define AVG_CPU             0
#define MAX_CPU             1
#define AVG_MEM             2
#define MAX_MEM             3

#define AVG_SESSION         4
#define MAX_SESSION         5

#define AVG_DIAM_TPS        6
#define MAX_DIAM_TPS        7
#define AVG_DIAM_RATE       8
#define MIN_DIAM_RATE       9

#define AVG_ROME_TPS        10 
#define MAX_ROME_TPS        11 
#define AVG_ROME_RATE       12
#define MIN_ROME_RATE       13

#define AVG_GTPP_TPS        14
#define MAX_GTPP_TPS        15
#define AVG_GTPP_RATE       16
#define MIN_GTPP_RATE       17

#define AVG_NSTEP_TPS       18
#define MAX_NSTEP_TPS       19
#define AVG_NSTEP_RATE      20
#define MIN_NSTEP_RATE      21

#define AVG_RADIUS_TPS      22
#define MAX_RADIUS_TPS      23
#define AVG_RADIUS_RATE     24
#define MIN_RADIUS_RATE     25

#if 1	/* jhpark */
#define AVG_NSCP_TPS      26
#define MAX_NSCP_TPS      27
#define AVG_NSCP_RATE     28
#define MIN_NSCP_RATE     29
#endif

#if 0	/* jhpark */
#define MAX_LOAD_STAT_FIELD 26 
#else
#define MAX_LOAD_STAT_FIELD 30 
#endif

/*************************************************/
/****    NSTEP Statistic Structure           *****/
/*************************************************/
#if 0
enum nstep_stat_cmd {
	NSTEP_STAT_ADD			= 0,
	NSTEP_STAT_DELETE		= 1,
	NSTEP_STAT_UP_MDN		= 2,
	NSTEP_STAT_UP_GEN		= 3,
	NSTEP_STAT_UP_IMSI		= 4,
	NSTEP_STAT_UP_IMEI		= 5,
	NSTEP_STAT_QUERY		= 6,
	NSTEP_STAT_CMD_MAX		= 7 
};
#endif
enum nstep_stat_cmd {
	NSTEP_STAT_ADD			= 0,
	NSTEP_STAT_DELETE		= 1,
	NSTEP_STAT_UPT_GEN		= 2,
	NSTEP_STAT_UPT_MDN		= 3,
	NSTEP_STAT_QUERY		= 4,
	NSTEP_STAT_ADD_SOC		= 5,
    NSTEP_STAT_DEL_SOC		= 6,
 	NSTEP_STAT_UPT_SOC		= 7,
	NSTEP_STAT_QRY_SOC		= 8,	
	NSTEP_STAT_CMD_MAX		= 9 
};

enum nstep_stat_reason {
	WISE_ATTEMPT			= 0,
	WISE_SUCCESS			= 1,
	WISE_FAIL				= 2,
//	WISE_TIMEOUT			= 3,
	WISE_FORMAT				= 3,
	WISE_KEY_ALREADY_REGI	= 4,
	WISE_KEY_NOT_REGI		= 5,
	WISE_INTERNAL			= 6,
	WISE_PLAS_FAIL			= 7,
	NSTEP_REASON_MAX		= 8
};

enum rome_stat_cmd {
//	SMPP_STAT_BIND		= 0,	//by KHL
	SMPP_STAT_SUBMIT	= 0,		
	SMPP_STAT_CMD_MAX	= 1
};

// 응답이 전달 되면 status 과 무관하게 Sucess 이다.
enum rome_stat_reason {
	SMPP_ATTEMPT		 = 0,
	SMPP_SUCCESS		 = 1,
	SMPP_FAIL			 = 2,
	SMPP_ROME_FORMAT	 = 3,
	SMPP_ROME_TIMEOUT	 = 4,
	SMPP_ROME_NO_CONN	 = 5,
	SMPP_ROME_SUBMIT_ERR = 6,
	SMPP_ROME_INTERNAL	 = 7,
	SMPP_REASON_MAX		 = 8
};

enum port_stat_item {
	PORT_STAT_RX_OCTET	= 0,
	PORT_STAT_RX_PACKET	= 1,
	PORT_STAT_RX_ERROR	= 2,
	PORT_STAT_RX_DROP	= 3,
	PORT_STAT_TX_OCTET  = 4,
	PORT_STAT_TX_PACKET = 5,
	PORT_STAT_TX_ERROR  = 6,
	PORT_STAT_TX_DROP   = 7,
	PORT_STAT_ITEM_MAX  = 8
};

enum radius_stat_cmd {
	RADIUS_STAT_ACCESS	= 0,
	RADIUS_STAT_START	= 1,
	RADIUS_STAT_INTERIM = 2,
	RADIUS_STAT_STOP	= 3,
	RADIUS_STAT_CMD_MAX	= 4 
};

enum radius_stat_reason {
	RADIUS_ATTEMPT			= 0,
	RADIUS_SUCCESS			= 1,
	RADIUS_FAIL				= 2,
	RADIUS_FORMAT			= 3,
	RADIUS_AUTH_FAIL		= 4,
	RADIUS_SUSPEND			= 5,
	RADIUS_INTERNAL			= 6,
	RADIUS_REASON_MAX		= 7
};


enum gtpp_stat_msg {
	GTPP_STAT_ECHO			= 0,		// EchoRequest
	GTPP_STAT_VER			= 1,		// VersionNotSupported
	GTPP_STAT_NODE 			= 2,		// NodeAliveRequest
	GTPP_STAT_REDIR			= 3,		// RedirectionRequest
	GTPP_STAT_DRTR			= 4, 		// DataRecordTransferRequest
	GTPP_STAT_MAX			= 5 
};

enum gtpp_stat_reason {
	GTPP_ATTEMPT			= 0,
	GTPP_SUCCESS			= 1,
	GTPP_FAIL				= 2,
	GTPP_FORMAT_ERROR		= 3,
	GTPP_NO_RESRC_AVAIL		= 4,
	GTPP_DECODE_ERROR		= 5,
	GTPP_INTERNAL			= 6,
	GTPP_REASON_MAX			= 7
};


// Diameter command
enum diameter_stat_cmd {
	DIAMETER_STAT_UDR		= 0,
	DIAMETER_STAT_PUR		= 1,
	DIAMETER_STAT_SNR   	= 2,
	DIAMETER_STAT_PNR		= 3,
	DIAMETER_STAT_CMD_MAX	= 4 
};

// Diameter reason
enum diameter_stat_reason {
	FIELD_DIAMETER_ATTEMPT				= 0,
	FIELD_DIAMETER_SUCCESS				= 1,
	FIELD_DIAMETER_FAIL					= 2,
	FIELD_DIAMETER_UNABLE_TO_DELIVER 	= 3,
	FIELD_DIAMETER_TIMEOUT				= 4,
	FIELD_DIAMETER_NO_SESSION			= 5,
	FIELD_DIAMETER_NO_COMPLY			= 6,
	FIELD_DIAMETER_NO_USER				= 7,
	FIELD_DIAMETER_TOO_BUSY				= 8,
	FIELD_DIAMETER_ETC					= 9,
	FIELD_DIAMETER_REASON_MAX			= 10
};

enum pcc_stat_type {
	PCC_STAT_USE		= 0,
	PCC_STAT_TYPE_MAX	= 1 
};

enum pcc_stat_reason {
	FIELD_PCC_TOT				= 0,
	FIELD_PCC_UP				= 1,
	FIELD_PCC_DOWN				= 2,
	FIELD_PCC_OVER				= 3,
	FIELD_PCC_ERR				= 4
};

/* 2017.03.08 bhj add NSCP */
enum nscp_stat_cmd {
	NSCP_STAT_SETUP         = 0,
	NSCP_STAT_MODE_QUERY    = 1,
	NSCP_STAT_CMD_MAX       = 2
};

enum nscp_stat_reason {
	NSCP_ATTEMPT            = 0,
	NSCP_SUCCESS            = 1,
	NSCP_FAIL               = 2,
	NSCP_FORMAT             = 3, //2200,2300,2400
	NSCP_IMSI_MDN_NOT_REGI  = 4, //4102,4002
	NSCP_NOT_ATTACH         = 5, //5300
	NSCP_3G_LOCATION_REG    = 6, // 5301 
	NSCP_INTERNAL           = 7, //1103,2100,4103,5000
	NSCP_HLR_INTERNAL       = 8, //1000,1001,1002,1100,1101,1102
	NSCP_MANDA_PARA_MISS    = 9, // error code 4  
	NSCP_TRANS_NOT_FOUND    = 10,// error code 5 
	NSCP_LOCAL_TIMEOUT      = 11,// REQ 보낸 후, 응답이 없는 case
	NSCP_OPERATION_TIMEOUT  = 12,// error code 8  
	NSCP_ETC                = 13,// error code 0,1,2,3,6,7  
	NSCP_REASON_MAX         = 14
};

enum mg_stat_cmd {
    MG_STAT_ADD          = 0,
    MG_STAT_DELETE       = 1,
    MG_STAT_UPT_GEN      = 2,
    MG_STAT_STATUS       = 3,
    MG_STAT_QRY		     = 4,
    MG_STAT_CMD_MAX      = 5
};

enum mg_stat_reason {
    MG_ATTEMPT            = 0,
    MG_SUCCESS            = 1,
    MG_FAIL               = 2,
    MG_FORMAT             = 3,
    MG_TIMEOUT			  = 4,
    MG_KEY_NOT_REGI       = 5,
    MG_INTERNAL           = 6,
    MG_DEST_FAIL          = 7,
    MG_REASON_MAX        = 8
};

enum ftpib_stat_cmd{
	    FTPIB_STAT_ADD          = 0
			,   FTPIB_STAT_CHANGE       = 1
			,   FTPIB_STAT_DELETE       = 2
			,   FTPIB_STAT_ADJUST       = 3
			,   FTPIB_STAT_CHG_LIMIT    = 4
			,   FTPIB_STAT_PLTE_AUTH    = 5
			,   FTPIB_STAT_NAS_AUTH     = 6
			,   FTPIB_STAT_ETC          = 7
			,   FTPIB_STAT_CMD_MAX      = 8
};

enum ftpib_stat_reason {
	    FTPIB_ATTEMPT       = 0
			,   FTPIB_SUCCESS       = 1
			,   FTPIB_FAIL          = 2
			,   FTPIB_FIELD_ERR     = 3
			,   FTPIB_NOT_FOUND     = 4
			,   FTPIB_DB_FAIL       = 5
			,   FTPIB_SYS_FAIL      = 6
			,   FTPIB_REASON_MAX    = 7
};

/************** add by hangkuk at 2020.05.07 ***************/

/*************************************************/
/****    RADIUS OP Statistic Structure        *****/
/*************************************************/

/*************** RADIUS_OP ********************/
enum radius_op_stat_reason {
	RADIUS_OP_ATTEMPT			= 0,
	RADIUS_OP_SUCCESS           = 1,
	RADIUS_OP_FAILURE           = 2,
	RADIUS_OP_DISCARD      		= 3,
	RADIUS_OP_BAD_FORMAT     	= 4,
	RADIUS_OP_NOT_FOUND       	= 5,
	RADIUS_OP_AUTH_FAIL    		= 6,
	RADIUS_OP_INTERNAL_ERR      = 7,
	RADIUS_OP_OVERLOADED        = 8,
	RADIUS_OP_REASON_MAX        = 9 
};

/*************************************************/
/****    RADIUS ACCESS HOST Statistic Structure      *****/
/*************************************************/
/* RADIUS_ACCESS_HOST */
enum radiusaccessophost_stat_item {
	RADIUSACCESSOPHOST_INIT_ATT      = 0,
	RADIUSACCESSOPHOST_INIT_SUCC     = 1,
};

/************** end of hangkuk at 2020.05.07 ***************/

// TPS Shm
typedef struct {
	int     curTps;		// attempt count
} TpsInfo;

typedef struct {
	TpsInfo tpsInfo[MAX_STAT_TYPE_NUM];	
} SFM_TpsInfoList;	//from comm_msgtypes.h


/** add 2016.0609 */

#pragma pack(1)
typedef struct {

#if 0
   unsigned int         addr;
#else
      char                 addr[STM_IPSTR_LEN];
#endif
    unsigned int         noDataRx;       /* number DATAs received */
    unsigned int         noDAckRx;       /* number SACKs received */
   unsigned int         noHBeatRx;      /* number of HEARTBEATs received */
     unsigned int         noHBAckRx;      /* number of HBEAT_ACKs received */
    unsigned int         bytesRx;        /* bytes received */
} LocalPath;
#pragma pack()

#pragma pack(1)
typedef struct {

#if 0
   unsigned int         addr;
#else
      char                 addr[STM_IPSTR_LEN];
#endif
    unsigned int         noDataTx;       /* number DATAs sent */
   unsigned int         noDataReTx;     /* number DATAs resent */
   unsigned int         noDataDrp;      /* number DATAs send drop */
   unsigned int         noDAckTx;       /* number of SACKs sent */
   unsigned int         noHBeatTx;      /* number of HEARTBEATs sent */
   unsigned int         noHBAckTx;      /* number of HBEAT_ACKs sent */
   unsigned int         bytesTx;        /* bytes sent */
} DestPath;
#pragma pack()


#define MAX_NET_ADDRS    5

#pragma pack(1)
typedef struct {

	   unsigned int nmb;
	      LocalPath LPath[MAX_NET_ADDRS];
} LocalPathStatistics;
#pragma pack()

#pragma pack(1)
typedef struct {

	  unsigned int nmb;
	 DestPath DPath[MAX_NET_ADDRS];
} DestPathStatistics;
#pragma pack()

#pragma pack(1)
typedef struct {
	 unsigned int AssocId;
	  LocalPathStatistics LPathstat;
	   DestPathStatistics  DPathstat;
} AssSts;
#pragma pack()

#define MAX_ASSO_NUM  20

#pragma pack(1)
typedef struct {

	   unsigned char nmb;
	      AssSts  Asso[MAX_ASSO_NUM];
} STM_SIGAssoStatisticMsgType;
#pragma pack()

#endif /*__STM_MSGTYPES_H__*/
