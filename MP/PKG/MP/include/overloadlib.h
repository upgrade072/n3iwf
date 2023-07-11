#ifndef __OVERLOADLIB_H__
#define __OVERLOADLIB_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "sysconf.h"
#include "conflib.h"
#include "loglib.h"
#include "comm_util.h"
#include "sfm_msgtypes.h"
#if 0 /* edited by hangkuk at 2020.05.12 */
#include "comm_almsts_msgcode.h"
#else
#include "almlib.h"
#endif /* edited by hangkuk at 2020.05.12 */
#include "comm_msgtypes.h"

#define	_MAX_PATH				256

#define OVLD_CONF_FILE          "data/overload.conf"
#define OVLD_MONITORING_LOG     "log/OVERLOAD/ovld_mon_log"

#define OVLDLIB_MODE_MONITOR   0x00000001  // monitoring process
#define OVLDLIB_MODE_SVC       0x00000002  // service process
#define OVLDLIB_MODE_CALL_CTRL 0x00000100  // tps 임계치 초과시 호 제어 필요.
#define OVLDLIB_MODE_DONT_CTRL 0x00000200  // 호 제어는 필요 없고 tps 산출만한다.

/* For NWUCP */
#define OVLD_CTRL_SVC_NGAP		0   /* NWUCP OP : INITIAL_CTX_SETUP PDU_SESS_SETUP DOWNLINK_NAS_TRANS */
#define OVLD_CTRL_SVC_TCP		1   /* NWUCP OP : UPLINK_NAS_TRANS */
#define OVLD_CTRL_SVC_EAP		2   /* NWUCP OP : EAP_INIT */

#define NUM_OVLD_CTRL_SVC   5 
#define OVLD_CTRL_CLEAR     0
#define OVLD_CTRL_SET       1
#define NUM_OVLD_CTRL_OPER  32
#define OVLD_MAX_MON_PERIOD 30
#define NUM_OVLD_CTRL_APPL	SYSCONF_MAX_APPL_NUM

typedef struct {
    int     mon_period;         // Avarage TPS 산출 및 Alarm,Status 메시지 출력 주기. 초 단위 시간.
    int     cpu_duration;       // CPU 과부하 발생,해지 지속시간. 초 단위 시간.
    int     cpu_limit_occur;    // CPU 과부하 발생 임계치. occur값 이상 duration 시간 지속되면 과부하 발생
    int     cpu_limit_clear;    // CPU 과부하 해제 임계치. 과부하 상태에서 clear값 이하로 duration 시간 지속되어야 해제.
	int     mem_duration;       // CPU 과부하 발생,해지 지속시간. 초 단위 시간.
	int     mem_limit_occur;    // CPU 과부하 발생 임계치. occur값 이상 duration 시간 지속되면 과부하 발생
	int     mem_limit_clear;    // CPU 과부하 해제 임계치. 과부하 상태에서 clear값 이하로 duration 시간 지속되어야 >해제.

    char    svc_name[NUM_OVLD_CTRL_SVC][16];
    int     tps_limit_normal[NUM_OVLD_CTRL_SVC];
    int     tps_limit_ovld[NUM_OVLD_CTRL_SVC];
    char    ctrl_oper_list[NUM_OVLD_CTRL_SVC][NUM_OVLD_CTRL_OPER][32]; // 서비스별 operation별 호 제한 대상 목록
} T_OverloadConfig;

typedef struct {
    volatile int     svc_tps[NUM_OVLD_CTRL_SVC][NUM_OVLD_CTRL_APPL];  // 서비스별 각 application 블록별 current attempt tps
    volatile int     fail_tps[NUM_OVLD_CTRL_SVC][NUM_OVLD_CTRL_APPL]; // 서비스별 각 application 블록별 current fail 처리한 tps
    volatile int     ctrl_tps[NUM_OVLD_CTRL_SVC][NUM_OVLD_CTRL_APPL]; // 임계치 초과로 호 제한 처리된 tps
} T_OverloadCurrTPS;

typedef struct {
    int     curr_level;
    int     occur_try;  // 정상->해지로 판단하기 위한 카운터
    int     clear_try;  // 발생->해지로 판단하기 위한 카운터
	/* for mem */
	int     mem_curr_level;
	int     mem_occur_try;  // 정상->해지로 판단하기 위한 카운터
	int     mem_clear_try;  // 발생->해지로 판단하기 위한 카운터

    int     last_svc_tps[NUM_OVLD_CTRL_SVC];  // 1초전 서비스별 tps
    int     last_fail_tps[NUM_OVLD_CTRL_SVC]; // 1초전 서비스별 fail 처리된 tps
    int     last_ctrl_tps[NUM_OVLD_CTRL_SVC]; // 1초전 서비스별 호 제한 처리된 tps
    int     period_svc_tps[NUM_OVLD_CTRL_SVC][OVLD_MAX_MON_PERIOD];  // 최근 mon_period 기간동안 tps 수치 보관
    int     period_fail_tps[NUM_OVLD_CTRL_SVC][OVLD_MAX_MON_PERIOD]; // 최근 mon_period 기간동안 fail 처리한 count 보관
    int     period_ctrl_tps[NUM_OVLD_CTRL_SVC][OVLD_MAX_MON_PERIOD]; // 최근 mon_period 기간동안 호 제한된 count 보관
    int     recent_svc_tps[NUM_OVLD_CTRL_SVC]; // 최근 mon_period 기간동안 평균TPS. 1초마다 최근 값으로 계산된다.
    int     recent_succ_rate[NUM_OVLD_CTRL_SVC]; // 최근 mon_period 기간동안 성공율. 1초마다 최근 값으로 계산된다.
    time_t  last_occur_time; // 마지막 발생 시간
    time_t  last_clear_time; // 마지막 해지 시간

	/* for mem */
	time_t  mem_last_occur_time; // 마지막 발생 시간
	time_t  mem_last_clear_time; // 마지막 해지 시간

    time_t  last_monitor_time;  // 과부하 감시 프로세스가 마지막으로 기록한 시간.
                                // 과부하 감시 프로세스가 죽어 있는 경우 curr_tps가 reset 되지 않아서 오동작하게 되므로
                                // 호 제어 여부 판단 시 현재시간과 duration 시간 이상 차이 나면 무조건 정상 처리한다.
} T_OverloadStatus;

typedef struct {
    T_OverloadConfig    cfg;
    T_OverloadStatus    sts;
    T_OverloadCurrTPS   curr0; // 초 단위시간을 홀,짝수로 나누어 기록한다. 호처리 블록에서 count하고 olcd 블록에서 수집할때 read/write 분리
    T_OverloadCurrTPS   curr1; // 짝수 초에는 curr0에, 홀수 초에는 curr1에 기록한다.
} T_OverloadInfo;

extern char ovld_svc_list[NUM_OVLD_CTRL_SVC][16];

extern int ovldlib_init (char *myAppName);
extern int ovldlib_attach_shm (void);
extern int ovldlib_isOvldCtrl (time_t now, int mode, int svcId, char *operName);
extern int ovldlib_increaseFailCnt (int svcId);


#endif //__OVERLOADLIB_H__
