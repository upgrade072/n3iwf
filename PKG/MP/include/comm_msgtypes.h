#ifndef __COMM_MSGTYPES_H__
#define __COMM_MSGTYPES_H__

/*------------------------------------------------------------------------------
// ��� �ý��ۿ��� �������� ���Ǵ� �޽����� ���� ������ �����Ѵ�.
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
	char		segFlag; /* / segment flag -> �� �޽����� �ʹ� ��� �ѹ��� ������ ���Ҷ� ���ȴ�.*/
	char		seqNo;   /* / sequence number -> segment�� ��� �Ϸù�ȣ */
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
#define MML_MAX_CMD_COUNT			10					/* MMC ��� �ɰ� ���� ���� */
#define MML_MAX_CMD_NAME_LEN		32
#define MML_MAX_PARA_CNT			48					//90	 /* 2015.06.09. plas */
#define MML_MAX_PARA_NAME_LEN		32 
#define MML_MAX_PARA_VALUE_LEN		256					//128	/* 2015.06.09. plas*/

/* MMC ��ɾ� ��� �� continue Flag */
#define RES_SUCCESS					0
#define RES_FAIL					(-1)
#define FLAG_COMPLETE				0
#define FLAG_CONTINUE				1

typedef struct mmc_handler {
	char	cmdName[MML_MAX_CMD_NAME_LEN];
	int		(*func)(struct ixpc_qmsg *);
} MmcHdlrVector;

/* 
 * MMCD�� Application���� ������ ��ɾ� ó�� �䱸 �޽��� 
 * */
typedef struct mml_para {
    char    paraName[MML_MAX_PARA_NAME_LEN];
    char    paraVal[MML_MAX_PARA_VALUE_LEN];
} __attribute__ ((packed)) CommPara;

typedef struct mml_req_head {
    unsigned short  sockFd;		// rmi�� ������ socket FD : trace �뵵�� �߰���. jumaeng 2007.08.22
	unsigned short	mmcdJobNo; // mmcd���� �����ϴ� key�� -> result�� �ݵ�� ������ ���� �־�� �Ѵ�.
	unsigned short 	paraCnt;  // �Էµ� parameter�� ���� 
	char	 cmdName[MML_MAX_CMD_NAME_LEN]; 
	CommPara para[MML_MAX_PARA_CNT];
// �Ķ���� value�� string���� ����. ex) 10���� 123�� string "123"���� ����.
// �Ķ���ʹ� command file�� ��ϵ� ������� ���ʷ� ����.
// �Էµ��� ���� optional �Ķ���Ϳ��� NULL�� ����.
} __attribute__ ((packed)) MMLReqMsgHeadType;

typedef struct mml_request {
	MMLReqMsgHeadType	head;
} __attribute__ ((packed)) MMLReqMsgType;


/* 
 * Application�� MMCD�� ������ ��ɾ� ó�� ��� �޽��� 
 * */
typedef struct mml_res_head {
	unsigned short	extendTime; /* / not_last�� ��� ���� �޽������� timer ����ð�(��) -> mmcd���� extendTime �ð���ŭ timer�� �����Ų��.*/
	unsigned short	mmcdJobNo;  /* / mmcd���� �����ϴ� key�� -> result�� �ݵ�� ������ ���� �־�� �Ѵ�.*/
	char			resCode;    /* / ��ɾ� ó�� ��� -> success(0), fail(-1) */
	char			contFlag;   /* / ������ �޽��� ���� ǥ�� -> last(0), not_last(1) */
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
} IFB_KillPrcNotiMsgType; /* killsys�� ���Ǵ� ����Ÿ ���� */


#define COMM_MAX_IPCBUF_LEN          8192

/* COND ���� GUI�� ������ ���Ǵ� ����ü�� */

#define FIMD_MAX_ALM_CODE_LEN           6
#define FIMD_MAX_STS_CODE_LEN           FIMD_MAX_ALM_CODE_LEN
#define FIMD_MAX_ALM_DATE_LEN           24  //yyyy-MM-dd HH:mm:ss.SS , 2009-04-17 10:57:30.22
 
#define STMD_MAX_STAT_CODE_LEN          FIMD_MAX_ALM_CODE_LEN
#define STMD_MAX_STAT_DATE_LEN          20  //yyyy-MM-dd HH:mm , 2009-04-17 10:55
 
//ALM �޽��� ���
typedef struct {
    char            almCode[FIMD_MAX_ALM_CODE_LEN];     // ����ڵ�, ���ڿ��� ǥ��(4�ڸ�)
    char            date[FIMD_MAX_ALM_DATE_LEN];        // �߻��ð�(yyyy-MM-dd HH:mm:ss.SS , 2009-04-17 10:57:30.22)
    char            sysName[COMM_MAX_NAME_LEN];         // �ý��۸�
    unsigned char   level;                              // ��ֵ��
    unsigned char   occurFlag;                          // ��ֹ߻�/���� ����
} CondAlmHeaderType;
 
//���� ���� �޽��� ���
typedef struct {
    char            stsCode[FIMD_MAX_STS_CODE_LEN];     // �����ڵ�, ���ڿ��� ǥ��(4�ڸ�)
    char            date[FIMD_MAX_ALM_DATE_LEN];        // �߻��ð�(yyyy-MM-dd HH:mm:ss.SS , 2009-04-17 10:57:30.22)
    char            sysName[COMM_MAX_NAME_LEN];         // �ý��۸�
} CondStsHeaderType;
 
 
//��� �޽���
typedef struct {
    unsigned char   statType;                           // �������
    unsigned char   jobType;                            // �۾����
    unsigned char   timeType;                           // �ð�Ÿ��
    char            statCode[STMD_MAX_STAT_CODE_LEN];   // ����ڵ�, ���ڿ��� ǥ��(4�ڸ�)
    char            startDate[STMD_MAX_STAT_DATE_LEN];  // �����۽ð�(yyyy-MM-dd HH:mm , 2009-04-17 13:45)
    char            endDate[STMD_MAX_STAT_DATE_LEN];    // ��賡�ð�(yyyy-MM-dd HH:mm , 2009-04-17 13:45)
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
	int         resendCnt; // ���� ���� ������
	int		    loc_sts;
	char        mdn[19];
} NSCPMON_MSG;

#define DEF_NSCPMON_REQ_SIZE sizeof(NSCPMON_MSG)


#endif /*__COMM_MSGTYPES_H__*/
