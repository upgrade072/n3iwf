#ifndef __SFM_MSGTYPES_H__
#define __SFM_MSGTYPES_H__

#include "sfmconf.h"
#include "extconn.h"

#define SHM_LOC_SADB	"SHM_LOC_SADB"

/* 주기적으로 OMP의 FIMD로 공통적인 정보(mem, cpu, disk, lan, process)를 전달하기 위한 데이타 구조 */
#pragma pack(1)
typedef struct {
	char        	name[COMM_MAX_NAME_LEN];
	char        	sub_name[COMM_MAX_NAME_LEN];
	unsigned int   	usage;
} SFM_SysCommItemSts;
#pragma pack()

#pragma pack(1)
typedef struct {
	unsigned short  cpu_usage;
	unsigned int    usage[SFM_CPU_PART_CNT];
} SFM_SysCommCpuSts;
#pragma pack()

#pragma pack(1)
typedef struct {
	unsigned short  mem_usage;
	unsigned int    usage[SFM_DISK_MEM_PART_CNT];
} SFM_SysCommMemSts;
#pragma pack()

#pragma pack(1)
typedef struct {
    char    		diskName[SFM_MAX_DISK_NAME];
    unsigned short  disk_usage;
    unsigned int    usage[SFM_DISK_MEM_PART_CNT];
} SFM_SysCommDiskSts;
#pragma pack()


#pragma pack(1)
typedef struct sfm_proc_status {
    char    processName[COMM_MAX_NAME_LEN];
    unsigned char   status;
    unsigned char   level;
} SFM_SysCommProcSts;
#pragma pack()


#pragma pack(1)
typedef struct {
    char    target_IPaddress[SFM_MAX_TARGET_IP_LEN];
    char    target_SYSName[SFM_MAX_LAN_NAME_LEN];
    unsigned char   status;
} SFM_SysCommLanSts;
#pragma pack()

#pragma pack(1)
typedef struct {
	int				qID;
	int				qKEY;
	char			qNAME[COMM_MAX_NAME_LEN];
	unsigned int	qNUM;
	unsigned int	cBYTES;
	unsigned int	qBYTES;
} SFM_SysCommQueSts;
#pragma pack()

#pragma pack(1)
typedef struct sfm_sys_common {
	unsigned char       active;
    unsigned char       lanCount;
    unsigned char       processCount;
    unsigned char       cpuCount;
    unsigned char       diskCount;
	unsigned char       queCount;
	char                host_name[SFM_MAX_TARGET_IP_LEN*2];  // add for eMTC
	char                server_id[SFM_MAX_TARGET_IP_LEN*4];  // add for eMTC 	
	SFM_SysCommCpuSts   loc_cpu_sts[SFM_MAX_CPU_CNT];
	SFM_SysCommMemSts   loc_mem_sts;
	SFM_SysCommDiskSts	loc_disk_sts[SFM_MAX_DISK_CNT];
	SFM_SysCommProcSts	loc_process_sts[SFM_MAX_PROC_CNT];
	SFM_SysCommLanSts	loc_lan_sts[SFM_MAX_LAN_CNT];
	SFM_SysCommQueSts   loc_que_sts[SFM_MAX_QUE_CNT];
} SFM_SysCommMsgType;
#pragma pack()

#define SHM_SYSCOMM_SIZE	sizeof (struct sfm_sys_common)

#define ACTIVE_SIDE     1
#define STANBY_SIDE     2

//-----------------------------------------------------------------
// SAMD -> FIMD    
//-----------------------------------------------------------------
#ifdef _SUNDB
enum {
    IDX_SUNDB_GMASTER           = 0,
    IDX_SUNDB_GLSNR             = 1,
    IDX_SUNDB_CYCLONE_MASTER    = 2,
    IDX_SUNDB_CYCLONE_SLAVE     = 3,
    MAX_SUNDB_IDX               = 4
};

#pragma pack(1)
typedef struct {
    unsigned char       dbCount;
    SFM_SysCommProcSts	loc_dbProc_sts[MAX_SUNDB_IDX];
} SFM_SpecInfoMsgType;
#pragma pack()




#else
#pragma pack(1)
typedef struct {
	char            name[SFM_MAX_REPL_NAME];    /* Unique */

#define SFM_DB_REPL_STOP        0                   /* replication이 stop된 상태 : 비정상 상태임 */
#define SFM_DB_REPL_CREATE      1                   /* replication table을 create한 상태. */
#define SFM_DB_REPL_STARTED     2                   /* replication이 start된 상태 : 정상 상태임 */
#define SFM_DB_REPL_SYNC_RUN    3                   /* 전체 table sync중... : 정상 상태임. replGap이 의미*/
#define SFM_DB_REPL_RETRY       4                   /* replication give_up됨. replGap 의미 없음 */
#define SFM_DB_REPL_GIVE_UP     SFM_DB_REPL_RETRY
#define SFM_DB_REPL_LAN_FAIL    5                   /*  db 모든 lan fail: 비정상 상태임 */
#define SFM_DB_REPL_GAP_OVER    6
#define SFM_DB_REPL_DISABLE     7                   /* db disconnect */

	unsigned char   status;                         /* Replication Status */
	unsigned int    gap;                        /* status SFM_DB_REPL_STARTED인 상태에서만 참조한다. */
	char            desc[COMM_MAX_NAME_LEN*4];
	int	checkOff;
} SFM_SpecInfoDBSyncMsgType;
#pragma pack(8)

#pragma pack(1)
typedef struct {
	unsigned char		dbCnt;
	unsigned char       dbReplCnt;
//#define MAX_DB_COUNT 16	//3	//2016.06.13.modified
#define MAX_DB_COUNT 30	//3	// change for eMTC
    SFM_SysCommProcSts	        loc_dbProc_sts[MAX_DB_COUNT];
	SFM_SpecInfoDBSyncMsgType   loc_dbRep_sts[SFM_MAX_REPL_CNT];
} SFM_SpecInfoMsgType;
#pragma pack()
#endif



#pragma pack(1)
typedef struct sfm_item_info {
	unsigned short		itemCnt;
#define MAX_ITEM_COUNT	30
	SFM_SysCommItemSts	item_sts[MAX_ITEM_COUNT];
} SFM_SepcItemMsgType;
#pragma pack()


//-----------------------------------------------------------------
// 외부 연동 OMP FIMD로 전달하는 구조체  ( SAMD -> FIMD ) 
//-----------------------------------------------------------------
typedef struct {
	int				type;
	char            name[COMM_MAX_NAME_LEN];
	int				id;
    char            pri_ipAddr[SFM_MAX_TARGET_IP_LEN];
    char            sec_ipAddr[SFM_MAX_TARGET_IP_LEN];
	int				port;
    int				maxConn;
    int				curConn;
    int			    status;						// 0: Normal, 1: Abnormal */
    char            desc[EXTCONN_NAME_LEN];	// 연동대상 시스템 이름
} SFM_ExtConnStatus;

typedef struct {
	int 			cnt;
	SFM_ExtConnStatus data[MAX_CONN_NUM];
} SFM_ExtConnStatusList;


//-----------------------------------------------------------------
// 외부 연동별 실시간TPS FIMD로 전달하는 구조체 ( STMD -> FIMD )
//-----------------------------------------------------------------
typedef struct {
    char            name[COMM_MAX_NAME_LEN];	//  chart name is matched to alm-cls name
    int				currTps;
} SFM_ExtConnTps;

typedef struct {
	int				num;
	SFM_ExtConnTps  data[MAX_EXTCONN_TYPE];	//	MAX_EXTCONN_TYPE의 IDX로 판단하기로 했음.
} SFM_ExtConnTpsList;

//-----------------------------------------------------------------
//  HW 상태 정보 ( SHMD -> FIMD )
//-----------------------------------------------------------------
#pragma pack(1)
typedef struct {
	unsigned char   status;
	unsigned char   level;
	unsigned char   reason;
	char            name[COMM_MAX_NAME_LEN];
	char            desc[COMM_MAX_NAME_LEN*4];
} SFM_SpecInfoHWSts;
#pragma pack()

#pragma pack(1)
typedef struct {
	SFM_SpecInfoHWSts hwcom[SFM_MAX_HPUX_HW_COM];
} SFM_SpecInfoHWMsgType;
#pragma pack()

//-----------------------------------------------------------------
// TPS OVERLOAD CONTROL 구조체 
//-----------------------------------------------------------------

#if 0
typedef struct {
	char	name[32];
	int     overload_run;   /* 0, 1 */
	int     limit;
	int     duration;
	int     ctrl_dur;
	int     overload_onoff;     /* 0: on 1: off */
	int     cur_tps;
} SFM_Ovreload; 


typedef struct {
#define OVER_LOAD_TYPE_AUTH		0
#define OVER_LOAD_TYPE_ACCT		1
#define MAX_OVER_LOAD_TYPE		10
	SFM_Ovreload	data[MAX_OVER_LOAD_TYPE];
} SFM_OvreloadCtrl;
#endif

#define MAX_OVERLOAD_TYPE_NUM	10

#define OVERLOAD_TPS_RADIUS				0
#define OVERLOAD_TPS_GTPP				1
#define OVERLOAD_TPS_DAIMETER			2
#define MAX_OVERLOAD_TPS_NUM			10
#define MAX_OVERLOAD_ALM_NUM			10


typedef struct {
    int             level;                      
    int             cpu_level;							// 0 : 과부하 해지, 1 : 과부하 발생
    int             mem_level;							// 0 : 과부하 해지, 1 : 과부하 발생
    int             tps_level[MAX_OVERLOAD_TPS_NUM];	// 0 : 과부하 해지, 1 : 과부하 발생

    time_t          cpu_occured_time;
    time_t          mem_occured_time;
    time_t          tps_occured_time[MAX_OVERLOAD_TPS_NUM];

    time_t          cpu_ctrl_time;
    time_t          mem_ctrl_time;
    time_t          tps_ctrl_time[MAX_OVERLOAD_TPS_NUM];

	int				alm_code[MAX_OVERLOAD_ALM_NUM];
    time_t          alm_time[MAX_OVERLOAD_ALM_NUM];

    // 설정 정보 
    int             max_cpu;
    int             max_mem;
    int             max_tps[MAX_OVERLOAD_TPS_NUM];    
    int             ctrl_duration;
    int             chck_duration;
    char            flag;                               // 0 : 과부하제어 미설정, 1 : 과부하 제어 설정 

} SFM_Overload;

typedef struct {
	SFM_Overload    data[MAX_OVERLOAD_TYPE_NUM];
} SFM_OverloadCtrl;



/* add 2016.06.09  */
#pragma pack(1)
typedef struct
{
	unsigned int         assoCnt;
	SFM_SS7AssoInfo_s      asso[SFM_MAX_SS7_ASSO_CNT];
} SFM_SpecInfoSS7AssoMsgType;
#pragma pack()

#endif /* __SFM_MSGTYPES_H__*/

