#ifndef __COMM_MSGTYPES_H__
#define __COMM_MSGTYPES_H__

/*------------------------------------------------------------------------------
// 모든 시스템에서 공통으로 사용되는 메시지에 대한 정보를 선언한다.
//------------------------------------------------------------------------------
*/
#define COMM_MAX_NAME_LEN	16
#define COMM_MAX_VALUE_LEN  100

#include "comm_msgid.h"
//#include "dbInfo.h"

typedef struct {
	unsigned char	pres;
	unsigned char	octet;
} SingleOctetType;

typedef struct FourOctetType {
    unsigned char   len;        /* length of octets */
    unsigned char   ostring[4]; /* contents */
} FourOctetType;

typedef struct {
	unsigned char	len;
#define MAX_OCTET_STRING_LEN	64
	unsigned char	ostring[MAX_OCTET_STRING_LEN];
} OctetStringType;


typedef long	genq_mtype_t;	// 2011.05.26
typedef struct general_qmsg{
	genq_mtype_t	mtype;
//#define MAX_GEN_QMSG_LEN		(32768-sizeof(long))  
#define MAX_GEN_QMSG_LEN		(81920-sizeof(long))  // for eMTC 
	char			body[MAX_GEN_QMSG_LEN];
} GeneralQMsgType;
#define GEN_QMSG_LEN(length)	(sizeof (genq_mtype_t) + length + 1)

typedef struct ixpc_header {
	int			msgId;   /* / message_id */
	char		segFlag; /* / segment flag -> 한 메시지가 너무 길어 한번에 보내지 못할때 사용된다.*/
	char		seqNo;   /* / sequence number -> segment된 경우 일련번호 */
	char        dummy[2];       // for alignment  - by mnpark 
#define	BYTE_ORDER_TAG	0x1234
	short		byteOrderFlag;
	short		bodyLen;
	char		srcSysName[COMM_MAX_NAME_LEN];
	char		srcAppName[COMM_MAX_NAME_LEN];
	char		dstSysName[COMM_MAX_NAME_LEN];
	char		dstAppName[COMM_MAX_NAME_LEN];
} IxpcQMsgHeadType;
#define IXPC_HEAD_LEN   sizeof(IxpcQMsgHeadType)

typedef struct ixpc_qmsg {
	IxpcQMsgHeadType	head;
#define MAX_IXPC_QMSG_LEN	((MAX_GEN_QMSG_LEN)-sizeof(IxpcQMsgHeadType))
	char				body[MAX_IXPC_QMSG_LEN];		/*	MMLReqMsgType	*/
} IxpcQMsgType;
#define	IXPC_QMSG_LEN(body_len)	(sizeof(IxpcQMsgHeadType) + body_len)

#define EMS_SYS_NAME	"OMP"

#define IXPC_SET_HEADER(head, mtype, msgid, src, dest, len)	\
do {													\
	(snd)->mtype = mtype;								\
	strcpy ((head)->srcSysName, mySysName);				\
	strcpy ((head)->srcAppName, src);					\
	strcpy ((head)->dstSysName, EMS_SYS_NAME);			\
	strcpy ((head)->dstAppName, dest);					\
														\
	(head)->msgId	= msgid;							\
	(head)->segFlag	= 0;								\
	(head)->seqNo	= 0;								\
	(head)->byteOrderFlag = BYTE_ORDER_TAG;				\
	(head)->bodyLen	= (len) + 1;						\
} while (0)


/* MMLMsg */
#define MML_CMD_LEN					32
#define MML_MAX_CMD_COUNT			10					/* MMC 결과 쪼개 보낼 갯수 */
#define MML_MAX_CMD_NAME_LEN		32
#define MML_MAX_PARA_CNT			48					//90	 /* 2015.06.09. plas */
#define MML_MAX_PARA_NAME_LEN		32 
#define MML_MAX_PARA_VALUE_LEN		256					//128	/* 2015.06.09. plas*/

/* MMC 명령어 결과 및 continue Flag */
#define RES_SUCCESS					0
#define RES_FAIL					(-1)
#define FLAG_COMPLETE				0
#define FLAG_CONTINUE				1

typedef struct mmc_handler {
	char	cmdName[MML_MAX_CMD_NAME_LEN];
	int		(*func)(struct ixpc_qmsg *);
} MmcHdlrVector;

/* 
 * MMCD가 Application으로 보내는 명령어 처리 요구 메시지 
 * */
typedef struct mml_para {
    char    paraName[MML_MAX_PARA_NAME_LEN];
    char    paraVal[MML_MAX_PARA_VALUE_LEN];
} __attribute__ ((packed)) CommPara;

typedef struct mml_req_head {
    unsigned short  sockFd;		// rmi로 접속한 socket FD : trace 용도로 추가함. jumaeng 2007.08.22
	unsigned short	mmcdJobNo; // mmcd에서 관리하는 key값 -> result에 반드시 동일한 값을 주어야 한다.
	unsigned short 	paraCnt;  // 입력된 parameter의 갯수 
	char	 cmdName[MML_MAX_CMD_NAME_LEN]; 
	CommPara para[MML_MAX_PARA_CNT];
// 파라미터 value는 string으로 들어간다. ex) 10진수 123은 string "123"으로 들어간다.
// 파라미터는 command file에 등록된 순서대로 차례로 들어간다.
// 입력되지 않은 optional 파라미터에는 NULL이 들어간다.
} __attribute__ ((packed)) MMLReqMsgHeadType;

typedef struct mml_request {
	MMLReqMsgHeadType	head;
} __attribute__ ((packed)) MMLReqMsgType;


/* 
 * Application이 MMCD로 보내는 명령어 처리 결과 메시지 
 * */
typedef struct mml_res_head {
	unsigned short	extendTime; /* / not_last인 경우 다음 메시지까지 timer 연장시간(초) -> mmcd에서 extendTime 시간만큼 timer를 연장시킨다.*/
	unsigned short	mmcdJobNo;  /* / mmcd에서 관리하는 key값 -> result에 반드시 동일한 값을 주어야 한다.*/
	char			resCode;    /* / 명령어 처리 결과 -> success(0), fail(-1) */
	char			contFlag;   /* / 마지막 메시지 여부 표시 -> last(0), not_last(1) */
	char            dummy[2];
	char			cmdName[MML_MAX_CMD_NAME_LEN]; /* / command name */
} __attribute__ ((packed)) MMLResMsgHeadType;

#define MAX_MML_MSG_LEN			(16 * 1024)
typedef struct mml_response {
	MMLResMsgHeadType	head;
//#	define MAX_MML_RESULT_LEN	(MAX_MML_MSG_LEN)-sizeof(MMLResMsgHeadType)
#	define MAX_MML_RESULT_LEN	((MAX_GEN_QMSG_LEN)-sizeof(IxpcQMsgHeadType))-sizeof(MMLResMsgHeadType)
	char				body[MAX_MML_RESULT_LEN];
} __attribute__ ((packed)) MMLResMsgType;
#define MML_RES_LEN(length) (sizeof (struct mml_res_head) + length + 1)


typedef struct {
    char    processName[16];
    int     Pid;
    int     type; //0:kill-prc, 1:start-prc
} IFB_KillPrcNotiMsgType; /* killsys시 사용되는 데이타 구조 */


#define COMM_MAX_IPCBUF_LEN          8192

/* COND 에서 GUI로 보랠때 사용되는 구조체들 */

#define FIMD_MAX_ALM_CODE_LEN           6
#define FIMD_MAX_STS_CODE_LEN           FIMD_MAX_ALM_CODE_LEN
#define FIMD_MAX_ALM_DATE_LEN           24  //yyyy-MM-dd HH:mm:ss.SS , 2009-04-17 10:57:30.22
 
#define STMD_MAX_STAT_CODE_LEN          FIMD_MAX_ALM_CODE_LEN
#define STMD_MAX_STAT_DATE_LEN          20  //yyyy-MM-dd HH:mm , 2009-04-17 10:55
 
//ALM 메시지 헤더
typedef struct {
    char            almCode[FIMD_MAX_ALM_CODE_LEN];     // 장애코드, 문자열로 표시(4자리)
    char            date[FIMD_MAX_ALM_DATE_LEN];        // 발생시간(yyyy-MM-dd HH:mm:ss.SS , 2009-04-17 10:57:30.22)
    char            sysName[COMM_MAX_NAME_LEN];         // 시스템명
    unsigned char   level;                              // 장애등급
    unsigned char   occurFlag;                          // 장애발생/해지 여부
} CondAlmHeaderType;
 
//상태 변경 메시지 헤더
typedef struct {
    char            stsCode[FIMD_MAX_STS_CODE_LEN];     // 상태코드, 문자열로 표시(4자리)
    char            date[FIMD_MAX_ALM_DATE_LEN];        // 발생시간(yyyy-MM-dd HH:mm:ss.SS , 2009-04-17 10:57:30.22)
    char            sysName[COMM_MAX_NAME_LEN];         // 시스템명
} CondStsHeaderType;
 
 
//통계 메시지
typedef struct {
    unsigned char   statType;                           // 통계종류
    unsigned char   jobType;                            // 작업방식
    unsigned char   timeType;                           // 시간타입
    char            statCode[STMD_MAX_STAT_CODE_LEN];   // 통계코드, 문자열로 표시(4자리)
    char            startDate[STMD_MAX_STAT_DATE_LEN];  // 통계시작시간(yyyy-MM-dd HH:mm , 2009-04-17 13:45)
    char            endDate[STMD_MAX_STAT_DATE_LEN];    // 통계끝시간(yyyy-MM-dd HH:mm , 2009-04-17 13:45)
} CondStatHeaderType;
 
#define COND_TRC_BODY_LEN               8000 - sizeof(CondTrcHeaderType)
#define COND_ALM_BODY_LEN               8000 - sizeof(CondAlmHeaderType)
#define COND_STS_BODY_LEN               8000 - sizeof(CondStsHeaderType)
#define COND_STAT_BODY_LEN              8000 - sizeof(CondStatHeaderType)

typedef struct {
    CondStsHeaderType   head;
    char                body[COND_STS_BODY_LEN];
} CondStsMsgType;
 
typedef struct {
    CondAlmHeaderType   head;
    char                body[COND_ALM_BODY_LEN];
} CondAlmMsgType;

// RETRY EVENT ALARM SNOOZE TIME
#define EVEVNT_ALM_SNOOZE_TIME		10


//  Alarm Structure Send To OMP 
typedef struct {
#if 0 /* edited by hangkuk at 2020.05.11 */
#define EVENT_ALM_OVERLOAD_CTRL         1120
#define EVENT_ALM_OFCS_RETRY         	1160
#define EVENT_ALM_FAILOVER              2050 
#define EVENT_ALM_SESSION_OVERLOAD      2060    // CDR SESSION usage overload
#define EVENT_ALM_FTP_FAIL      		2410    // FTP PUT FAIL 
#else
#define EVENT_ALM_OVERLOAD_CTRL         1120
#define EVENT_ALM_FAILOVER              2050
#define EVENT_ALM_TPS_OVERLOAD          2350
#define EVENT_ALM_FTP_FAIL              2410
#define EVENT_ALM_CPU_OVERLOAD          2360
#define EVENT_ALM_MEM_OVERLOAD          2361
#define EVENT_ALM_GEO_INVALID           2381
/* add 20200528 bhj FTPIB */
#define EVENT_ALM_EPCC_FTP_FAIL         2382
	    /* add night1700 emguib */
#define EVENT_ALM_EMGU_DISCONNECT       2383
/* schlee 20201209 */
#define EVENT_ALM_CERT_EXPIRE_WARN      2384

/* add 20201215 bhj BCSP */
#define EVENT_ALM_BCSP_CREATE_POLICY_FAIL 2385
#define EVENT_ALM_BCSP_CREATE_POOL_FAIL 2386
#define EVENT_ALM_BCSP_DELETE_POOL_FAIL 2387

#define EVENT_ALM_EMGPWD_DISCONNECT 2388
#define EVENT_ALM_ERS_DISCONNECTION 2389

#endif /* edited by hangkuk at 2020.05.11 */ 

	int     almCode;

	int		almLevel;	   // 1:Minor, 2:Mjaor, 3:Critial
	char    almDesc[256];  // OVEROAD          // HIMS-FTP-GET
	char    almInfo[256];  // OVER 1000 TPS    // FTP-FILE-GET ERROR or FILE LOADING ERROR
} AlmMsgInfo;

typedef struct t_nscpmon_msg {
	int         resendCnt; // 서비스 상태 설정값
	int		    loc_sts;
	char        mdn[19];
} NSCPMON_MSG;

#define DEF_NSCPMON_REQ_SIZE sizeof(NSCPMON_MSG)


#endif /*__COMM_MSGTYPES_H__*/
