/*
 * FILE : loglib.h
 * DESC : common log library
 * UPDATED: 
 *      2013.10.24 Donha
 *          - 프로세스별로 로그레벨 설정 가능하도록 변경함
 *          - sysconfig에 등록되지 않은 TOOL 등의 경우 로그처리 가능도록 함
 */
#ifndef	__COMM_LOGLIB_H__
#define	__COMM_LOGLIB_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <trclib.h>


#ifndef FL
#define	FL		__FILE__, __LINE__
#endif

#ifndef FFL
#define	FFL		__FILE__, __func__, __LINE__
#endif

#ifndef FUNCL
#define	FUNCL	__func__, __LINE__
#endif

#define	ERRLOG_DIR					"log/ERR_LOG"
#define	MSGLOG_DIR					"log/MSG_LOG"
#define TOOL_LOGPATH				"log/TOOL"

#define	LOGLIB_MAX_OPEN_FILE        64          // Process에서 최대로 open할 수 있는 log file 갯수

#define	LOGLIB_MODE_LIMIT_SIZE      0x00000001  // 화일 크기를 제한
#define	LOGLIB_MODE_DAILY           0x00000002  // 매일 새로운 화일을 생성
#define	LOGLIB_MODE_HOURLY          0x00000004  // 매시 새로운 화일을 생성
#define LOGLIB_MODE_5MIN			0x00000008  // 매5min 마다 새로운 파일 생성
#define	LOGLIB_FLUSH_IMMEDIATE      0x00010000  // 매번 fflush 한다
#define	LOGLIB_FNAME_LNUM           0x00020000  // 소스 파일이름과 line_number 기록
#define	LOGLIB_TIME_STAMP           0x00040000  // 시각(time_stamp)을 맨 앞에 기록
#define	LOGLIB_TIME_STAMP_FIRST     LOGLIB_TIME_STAMP


#define	LOGLIB_DEFAULT_SIZE_LIMIT   (1024*1024*4)	    // LOGLIB_MODE_LIMIT_SIZE의 경우 파일 크기 제한 (byte단위)
#define	LOGLIB_SIZE_LIMIT_MIN       1024            	// 파일 크기 허용 최소 (1 KB)
#define	LOGLIB_SIZE_LIMIT_MAX       (1024*1024*1024)	// 파일 크기 허용 최대 (1 GB)
#define	LOGLIB_DEFAULT_NUM_SUFFIX   200             	// LOGLIB_MODE_LIMIT_SIZE의 경우 같은 이름의 파일을 몇개까지 만들 것인지 정의
#define	LOGLIB_NUM_SUFFIX_MIN       3               	// num_suffxi 허용 최소
#define	LOGLIB_NUM_SUFFIX_MAX       100000          	// num_suffxi 허용 최대

#define TRCBUF_LEN  				102400 //  cht mod 2013.02.19 10240 -> 102400
#define TRCTMP_LEN  				1024

#define RED(data)       "[1;31m"data"[0m"
#define GREEN(data)     "[1;32m"data"[0m"
#define YELLOW(data)    "[1;33m"data"[0m"
#define BLUE(data)      "[1;34m"data"[0m"
#define PURPLE(data)    "[1;35m"data"[0m"
#define CYAN(data)      "[1;36m"data"[0m"
#define WHITE(data)     "[1;37m"data"[0m"

#define DRED(data)      "[0;31m"data"[0m"
#define DGREEN(data)    "[0;32m"data"[0m"
#define DYELLOW(data)   "[0;33m"data"[0m"
#define DBLUE(data)     "[0;34m"data"[0m"
#define DPURPLE(data)   "[0;35m"data"[0m"
#define DCYAN(data)     "[0;36m"data"[0m"
#define DWHITE(data)    "[0;37m"data"[0m"

#define LL0							0				// off
#define	LL1							1				// error
#define	LL2							2				// simple
#define	LL3							3				// detail
#define	LL4							4				// detail
#define	LL5							5				// detail
#define	LL_NUM						(LL5+1)

#define	LLE							LL1				// Error, This is Default

extern int trcLogId, trcErrLogId, trcMsgLogId, cdrLogId;

#ifndef ELI	
#define	ELI trcErrLogId
#endif
#ifndef DLI
#define	DLI	trcLogId
#endif
#ifndef MLI
#define	MLI	trcMsgLogId
#endif
#ifndef CLI
#define	CLI	cdrLogId
#endif

#if 0
/* Message Log 에 대한 On/Off 정보 */
#define	TOOL_LOG_IDX		0
#define STARTPRC_LOG_IDX    1
#define KILLPRC_LOG_IDX     2
#define IXPC_LOG_IDX        3
#define SAMD_LOG_IDX        4
#define SHMD_LOG_IDX        5
#define STMD_LOG_IDX        6
#define DBM_LOG_IDX         7  
#define PEM_LOG_IDX         8
#define MOIBA_LOG_IDX       9
#define LOAD_LOG_IDX        10

#define	TOOL_LOG_NAME		"TOOL"
#define STARTPRC_LOG_NAME   "startprc"
#define KILLPRC_LOG_NAME    "killprc"
#define IXPC_LOG_NAME       "IXPC"
#define SAMD_LOG_NAME       "SAMD"
#define SHMD_LOG_NAME       "SHMD"
#define STMD_LOG_NAME       "STMD"
#define DBM_LOG_NAME		"DBM"
#define PEM_LOG_NAME        "PEM"
#define MOIBA_LOG_NAME      "MOIBA"
#define LOAD_LOG_NAME       "LOAD"
#define CMD_LOG_NAME		"CMD"
#endif

#define LOG_PROC_MAX		64 
#define	MAX_PROC_NUM		LOG_PROC_MAX

#define SHM_LOGLEVEL		"SHM_LOG_LEVEL_INFO"
#define	SHM_LOGPROP			"SHM_LOGPROP"

#define LOG_OFF         0
#define LOG_ON          1

#define PROC_NAME_LEN   32
typedef struct log_info {
    char    procName[PROC_NAME_LEN];
    int     logFlag;        /* ON,OFF Flag */
    int     logLevel;       /* Log Level */
    int     expire;         /* Expire Time */
} T_LOG_INFO;

typedef struct log_level_info {
    T_LOG_INFO  logInfo[MAX_PROC_NUM];
} T_LOG_LEVEL_INFO;

typedef struct {
    char   		fname[256]; // file name; full path name
    int    		mode;
    int    		num_suffix; // MODE_LIMIT_SIZE -> 같은 이름의 파일 갯수 (맨끝 첨자 갯수); 0 이면 default 적용됨
    size_t 		limit_val;  // MODE_LIMIT_SIZE -> 한파일의 크기; 0 이면 default 적용됨
    char   		subdir_format[64];  // MODE_HOURLY 또는 MODE_DAILY 인 경우 sub directory 구조 체계
	char		appName[32];		// application name
} LoglibProperty;	// LoglibProperty;

typedef struct {
	int				level;				// log level
	int				*shm_level;			// 2014.12.29.added
    LoglibProperty  pro;				// log property
    FILE    		*fp;                // file pointer
    time_t  		lastTime;           // LOGLIB_MODE_DAILY, LOGLIB_MODE_HOURLY인 경우 마지막 기록 시각
    char    		daily_fname[256];   // LOGLIB_MODE_DAILY 인 경우 뒤에 현재날짜가 붙은 file name
    char    		hourly_fname[256];  // LOGLIB_MODE_HOURLY 인 경우 뒤에 현재날짜시간이 붙은 file name
    char    		dir_name[256];      // full path name 중 directory 부
    char    		file_name[64];      // file path name 중 file name 부
} LogInformation;

typedef struct {
	char	appName[32];
#define MAX_APPLOG_NUM	16	// APP 당 logId 개수
	LogInformation logInfo[MAX_APPLOG_NUM];
} LogInformationTable;

extern LogInformationTable		*myLogTbl;
extern struct log_level_info	*shm_loglvl;

extern int loglib_initLog (char *appName);
extern int loglib_attach_shm_level (int proc_index, int logId);
extern int loglib_openLog (LoglibProperty*);
extern int loglib_closeLog (int);
extern int logPrint (int, char*, int, char*, ...);
extern int loglib_checkLimitSize (int);
extern int loglib_checkDate (int);
extern int loglib_checkTimeHour (int);
extern int loglib_checkTime5m (int logId);
extern int loglib_setLogLevel(int logId, int level);
extern int loglib_getLogLevel(int logId);
extern const char* loglib_getLogLevelStr(int logId);
extern int loglib_get_shm_level (int logId);
extern int loglib_set_shm_level (int logId, int level);
extern void loglib_getLogFname(int logId, char *fname);
extern void __printf(char *fName, int lNum, char*fmt, ...);
extern int __lprintf(int logId, int level, const char *fName, int lNum, char*fmt, ...);
extern int __cprintf(int logId, int level, const char *fName, int lNum, char*fmt, ...);
extern void loglib_printMyLogTbl();
extern int loglib_makeDefaultFile(int logId);
extern void loglib_flush(int logId);
extern int system_popen(char *cmd);
extern int initLog(char *appName);

#define lprintf(logId, lvl, fl, fmt, arg...) \
    ({\
		if( myLogTbl != NULL && myLogTbl->logInfo[logId].level > LL0 && myLogTbl->logInfo[logId].level >= lvl ) \
			__lprintf(logId, lvl, fl, fmt, ##arg); \
    })

#define ERRLOG(lvl, fl, fmt, arg...) lprintf (trcErrLogId, lvl, fl, fmt, ##arg)
#define ELOG(fmt, arg...) lprintf (trcErrLogId, LLE, FL, fmt, ##arg)
#define TLOG(fmt, arg...) lprintf (trcErrLogId, LL3, FL, "[DETAIL] "fmt, ##arg)
#define MSGLOG(lvl, fl, fmt, arg...) lprintf (trcMsgLogId, lvl, fl, fmt, ##arg)
#define SLOG(fmt, arg...) lprintf (trcErrLogId, LL2, FL, fmt, ##arg)
#define DLOG(fmt, arg...) lprintf (trcErrLogId, LL4, FL, fmt, ##arg)
#define TRCLOG(lvl, fl, fmt, arg...) lprintf (trcLogId, lvl, fl, fmt, ##arg)

#define LOGLEVEL(logId)		\
		(myLogTbl && myLogTbl->logInfo[logId].shm_level ? \
					*(myLogTbl->logInfo[logId].shm_level) : myLogTbl->logInfo[logId].level) 

#define CHECK_LOG(logId, lvl)	(myLogTbl && LOGLEVEL(logId) >= lvl) 
	
#define log_print(logId, lvl, fl, fmt, arg...)						\
do {																\
	int log_level = LOGLEVEL (logId);								\
	if (myLogTbl != NULL && log_level > LL0 && log_level >= lvl)	\
		__lprintf(logId, lvl, fl, fmt, ##arg);						\
} while (0)

#define mlog_print(lvl, fmt, arg...)	log_print(trcMsgLogId, lvl, _FL_, fmt, ##arg)


/* for color log */
#define CERR(lvl, fl, fmt, arg...) \
    ({\
	 	if( myLogTbl != NULL && myLogTbl->logInfo[trcErrLogId].level > LL0 && myLogTbl->logInfo[trcErrLogId].level >= lvl ) \
			__cprintf(trcErrLogId, lvl, fl, fmt, ##arg); \
    })

/* use after main initial */
#define STARTLOG() \
	({\
	 	if( myLogTbl != NULL) \
	 		logPrint(ELI, FL, RED("============== %s STARTED! ==============")"\n", myAppName);\
	 })


/** for only OMP Source */
#define MSGLOGP(fmt, arg...) logPrint (trcMsgLogId,FL, fmt, ##arg)
#define ERRLOGP(fmt, arg...) logPrint (trcErrLogId,FL, fmt, ##arg)
#define TRCLOGP(fmt, arg...) \
		({\
			sprintf(trcBuf, fmt, ##arg); \
			trclib_writeLog (FL, trcBuf); \
		})
#define TRCLOGE(fmt, arg...) \
		({\
			sprintf(trcBuf, fmt, ##arg); \
			trclib_writeLogErr (FL, trcBuf); \
		})

#endif

