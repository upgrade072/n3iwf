#ifndef __SFMCONF_H__
#define __SFMCONF_H__

#include "sysconf.h"
/*#include "dsc.h"*/
#include "comm_msgtypes.h"

/*------------------------------------------------------------------------------
// Status & Fault Management ���õ� ������ �����Ѵ�.
//------------------------------------------------------------------------------*/

#undef SFM_MAX_TARGET_IP_LEN

/* �� Component�� �ִ� ���� ����
*/
#define SFM_MAX_PATH_CNT                2           // primary, secondary path
#define	SFM_MAX_CPU_CNT					64
#define SFM_CPU_PART_CNT        		4  /* USER, SYS, WIO, IDLE */
#define	SFM_MAX_DISK_CNT				16 // 8 -> 16
#define SFM_DISK_MEM_PART_CNT   		2  /* TOTAL, USAGE */
#define	SFM_MAX_LAN_CNT					SYSCONF_MAX_ASSO_SYS_NUM * 4 /* chg 2->4 alti,svcguard �� �߰� */
#define SFM_MAX_NIC_CNT					10
#define	SFM_MAX_PROC_CNT				SYSCONF_MAX_APPL_NUM
#define	SFM_MAX_QUE_CNT					SYSCONF_MAX_APPL_NUM
#define	SFM_MAX_DEV_CNT					2
#define SFM_MAX_OC7_CNT                 5
#define	SFM_MAX_DISK_NAME				COMM_MAX_NAME_LEN
#define	SFM_MAX_LAN_NAME_LEN			COMM_MAX_NAME_LEN
#define	SFM_MAX_TARGET_IP_LEN			16
#define	SFM_MAX_HW_COM					60
#define	SFM_MAX_HPUX_HW_COM					60
#define SFM_MAX_SS7_SP_CNT				256 /* ��ȣ�� �ִ� ����	*/
#define SFM_MAX_SS7_SS_SP_CNT			5   /* ��ȣ���� �ִ� ����ý��� ���� */
#define SFM_MAX_APP_NETMON				3   
#define	SFM_MAX_NET_MON_CNT				(SFM_MAX_APP_NETMON*3)
#define	SFM_MAX_TSU_CNT					4
#define	SFM_MAX_TSC_CNT					(SFM_MAX_TSU_CNT*2)
#define	SFM_LIMIT_ASSO_CNT				4
#define	SFM_ASSO_SEC_CNT				5
#define	SFM_MAX_SG_NET_CNT				SFM_ASSO_SEC_CNT * SFM_LIMIT_ASSO_CNT
//#define SFM_MAX_REPL_CNT                12  /* 20051122.onback.mnpark Replication Count */
#define SFM_MAX_REPL_CNT                30  // change for eMTC
#define SFM_MAX_REPL_NAME               40  /* 20051122.onback.mnpark Replication Name */
#define SFM_MAX_BOND_CNT                2

#define	SFM_MAX_SMS_NET_CNT				4 * 2

// SIGTRAN
//
#define SFM_MAX_SS7_SEP_CNT     4
#define SFM_MAX_SS7_ASP_CNT     20
#define SFM_MAX_SS7_ASPDPC_CNT      SFM_MAX_SS7_ASP_CNT
#define SFM_MAX_SS7_RTE_CNT     256
#define SFM_MAX_SS7_IPLST_CNT       5
#define SFM_MAX_SS7_ASSO_CNT        SFM_MAX_SS7_ASP_CNT

#define SFM_MAX_IPADDR_LEN              64

enum {                                                                                                                  
	SFM_PART_CPU_USER = 0,                                                                                              
	SFM_PART_CPU_SYS,                                                                                                   
	SFM_PART_CPU_WIO,                                                                                                   
	SFM_PART_CPU_IDLE,                                                                                                  
};                                                                                                                      
                                                                                                                        
// MEM, DISK  ��뷮                                                                                                    
enum {                                                                                                                  
	SFM_PART_TOTAL=0,                                                                                                   
	SFM_PART_USE,                                                                                                       
};    

/* ��� ������ �����ϱ� ���� ��
// - ���ǵ� ���� Ư���� �ǹ̸� ���� �ʰ�, �ܼ��� ���θ� �����ϴ� �뵵�θ� ���ǰ�
//  key���̳� index ���� �뵵�� ������ �ʴ´�.
// - alarm_history DB�� ��� �ִ� ������ ��ȸ�Ҷ� ���ȴ�.
*/
#define SFM_ALM_TYPE_CPU_USAGE			1
#define SFM_ALM_TYPE_MEMORY_USAGE		2
#define SFM_ALM_TYPE_DISK_USAGE			3
#define SFM_ALM_TYPE_LAN				4
#define SFM_ALM_TYPE_PROC				5
#define SFM_ALM_TYPE_TCP_CONN			6
#define SFM_ALM_TYPE_SS7_LINK			7
#define	SFM_ALM_TYPE_MP_HW				8
#define	SFM_ALM_TYPE_ONBKUP				9
#define SFM_ALM_TYPE_SS7_SP             10
#define SFM_ALM_TYPE_SS7_SS             11
#define SFM_ALM_TYPE_STAT               12
#define SFM_ALM_TYPE_DB_REP             13
#define SFM_ALM_TYPE_DB_REP_GAP         14
#define SFM_ALM_TYPE_FTP_FILE           15

#define SFM_ALM_OCCURED					1
#define SFM_ALM_CLEARED					0


/* ��� ���
*/
#define	SFM_ALM_NORMAL					0
#define	SFM_ALM_MINOR					1
#define	SFM_ALM_MAJOR					2
#define	SFM_ALM_CRITICAL				3

#define	SFM_ALM_MASKED					99

#define LINK_ALM_LEVEL      			SFM_ALM_MAJOR

// ���μ��� �� �Ϲ��� ������Ʈ�� ����
enum {
	SFM_STATUS_ALIVE        = 			0,
	SFM_STATUS_DEAD,
	SFM_STATUS_INIT,
};

/* LAN ����
*/
#define SFM_LAN_CONNECTED				0
#define SFM_LAN_DISCONNECTED			1



/* HPUX H/W ����
*/
#define SFM_HW_NOT_EQUIP     			0   /* must not equip */
#define SFM_HW_ABSENT        			1   /* must equip, but absent */
#define SFM_HW_DOWN          			2   /* must equip, down state */
#define SFM_HW_ONLINE        			3   /* must equip, online state */
#define SFM_HW_MIRROR        			4   /* must equip, mirroring */
#define SFM_HW_PORT_OFF      			5   /* must equip, port off */
#define SFM_HW_TEMPER_OK     			6    /* must equip, Temperature Normal */
#define SFM_HW_TEMPER_NOK    			7   /* must equip,  Tempurature abnormal*/




/* �� �ý��ۿ��� ȣó���� ���� �ٸ� �ý��۵�� TCP/IP�� ���ӵǾ�� �ϴ� �͵鿡 ����
//  ���� ���� ���¸� OMP�� �뺸�ϱ� ���� ����.
*/
#define SFM_MAX_TCP_CONN_CNT			8

#define	SFM_TCPCONN_INDEX_HLR2WISE		0	/* HLR���� �����Ǵ� WISE���� ���� */
#define	SFM_NUM_TCPCONN_HLR				1	/* HLR���� �����Ǵ� Connection ���� */

//challa(20090629)DIAMETER
#define SFM_MAX_NAME_LEN    			64
#define SFM_MAX_PEER_IP_LEN 			16

#define SFM_MAX_TCP_CNT					10

//================================================== //
// SAMD ���� �ʿ� �� ���μ��� ���� ��� ����ü
//================================================== //
typedef struct {
	char            procName[16];
	char            procArg[256];
	int             msgQkey;
	int             runCnt;
	pid_t           pid;
	char            exeFile[256];   // ���� ���������� full path
	char            startTime[32];
	unsigned char   old_status;
	unsigned char   new_status;
	unsigned char   mask;           // killsys�� ���� ��� �����Ǿ� auto_restart���� �ʴ´�.
	char            sz_version[8];
}ProcessInfo;

typedef struct {
	char            procName[16];
	char            port[16];
	char            pid[16];
	int             status;
	char            version[32];
}DataBaseInfo;

typedef struct {
#define MAX_DBMS_CNT 					24	//3	//2016.06.13.modified
#ifdef _MYSQL
#define IDX_DBMS_MYSQL  0
#elif _ALTIBASE
#define IDX_DBMS_ALTIBASE 0
#endif
#define IDX_DBMS_REDIS1					1
#define IDX_DBMS_REDIS2					2
	int				cnt;
	DataBaseInfo	data[MAX_DBMS_CNT];
} DataBaseList;

/** 2016.06.09 **/
#pragma pack(1)
typedef struct {
   unsigned char    priority;                       /* 0:primary, 1:secondary */
#if 0
   unsigned int     destAddr;
   unsigned int     srcAddr;
#else
   char             destAddr[SFM_MAX_IPADDR_LEN];
   char             srcAddr[SFM_MAX_IPADDR_LEN];
#endif
   unsigned char    pathState;                     /* association path status */
} SFM_SS7PathInfo;
#pragma pack()


typedef struct
{
   unsigned char        assocId;                        /* association id */
   unsigned char        aspId;                          /* application server process id */
   unsigned short       dpc;                            /* destination point code */
#if 0
   unsigned int         destAddr;                       /* primary destination ip address */
#else
   char                 destAddr[SFM_MAX_IPADDR_LEN];
#endif
   unsigned int         destPort;                       /* destination port */
   unsigned char        locOutStrms;                    /* number of outbound streams support */
   unsigned char        assocState;                     /* association status */
   unsigned char        nmb;                            /* number of ip address list */
#if 0
   unsigned int         nAddr[SFM_MAX_SS7_IPLST_CNT];   /* list of ip address */
#else
   char                 nAddr[SFM_MAX_SS7_IPLST_CNT][SFM_MAX_IPADDR_LEN];   /* list of ip address */
#endif
   SFM_SS7PathInfo      path[SFM_MAX_PATH_CNT];         /* path state per association */
   unsigned char        mask;
} SFM_SS7AssoInfo_s;
#pragma pack()




#endif /*__SFMCONF_H__*/
