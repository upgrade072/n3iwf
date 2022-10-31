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
#define OVLDLIB_MODE_CALL_CTRL 0x00000100  // tps �Ӱ�ġ �ʰ��� ȣ ���� �ʿ�.
#define OVLDLIB_MODE_DONT_CTRL 0x00000200  // ȣ ����� �ʿ� ���� tps ���⸸�Ѵ�.

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
    int     mon_period;         // Avarage TPS ���� �� Alarm,Status �޽��� ��� �ֱ�. �� ���� �ð�.
    int     cpu_duration;       // CPU ������ �߻�,���� ���ӽð�. �� ���� �ð�.
    int     cpu_limit_occur;    // CPU ������ �߻� �Ӱ�ġ. occur�� �̻� duration �ð� ���ӵǸ� ������ �߻�
    int     cpu_limit_clear;    // CPU ������ ���� �Ӱ�ġ. ������ ���¿��� clear�� ���Ϸ� duration �ð� ���ӵǾ�� ����.
	int     mem_duration;       // CPU ������ �߻�,���� ���ӽð�. �� ���� �ð�.
	int     mem_limit_occur;    // CPU ������ �߻� �Ӱ�ġ. occur�� �̻� duration �ð� ���ӵǸ� ������ �߻�
	int     mem_limit_clear;    // CPU ������ ���� �Ӱ�ġ. ������ ���¿��� clear�� ���Ϸ� duration �ð� ���ӵǾ�� >����.

    char    svc_name[NUM_OVLD_CTRL_SVC][16];
    int     tps_limit_normal[NUM_OVLD_CTRL_SVC];
    int     tps_limit_ovld[NUM_OVLD_CTRL_SVC];
    char    ctrl_oper_list[NUM_OVLD_CTRL_SVC][NUM_OVLD_CTRL_OPER][32]; // ���񽺺� operation�� ȣ ���� ��� ���
} T_OverloadConfig;

typedef struct {
    volatile int     svc_tps[NUM_OVLD_CTRL_SVC][NUM_OVLD_CTRL_APPL];  // ���񽺺� �� application ��Ϻ� current attempt tps
    volatile int     fail_tps[NUM_OVLD_CTRL_SVC][NUM_OVLD_CTRL_APPL]; // ���񽺺� �� application ��Ϻ� current fail ó���� tps
    volatile int     ctrl_tps[NUM_OVLD_CTRL_SVC][NUM_OVLD_CTRL_APPL]; // �Ӱ�ġ �ʰ��� ȣ ���� ó���� tps
} T_OverloadCurrTPS;

typedef struct {
    int     curr_level;
    int     occur_try;  // ����->������ �Ǵ��ϱ� ���� ī����
    int     clear_try;  // �߻�->������ �Ǵ��ϱ� ���� ī����
	/* for mem */
	int     mem_curr_level;
	int     mem_occur_try;  // ����->������ �Ǵ��ϱ� ���� ī����
	int     mem_clear_try;  // �߻�->������ �Ǵ��ϱ� ���� ī����

    int     last_svc_tps[NUM_OVLD_CTRL_SVC];  // 1���� ���񽺺� tps
    int     last_fail_tps[NUM_OVLD_CTRL_SVC]; // 1���� ���񽺺� fail ó���� tps
    int     last_ctrl_tps[NUM_OVLD_CTRL_SVC]; // 1���� ���񽺺� ȣ ���� ó���� tps
    int     period_svc_tps[NUM_OVLD_CTRL_SVC][OVLD_MAX_MON_PERIOD];  // �ֱ� mon_period �Ⱓ���� tps ��ġ ����
    int     period_fail_tps[NUM_OVLD_CTRL_SVC][OVLD_MAX_MON_PERIOD]; // �ֱ� mon_period �Ⱓ���� fail ó���� count ����
    int     period_ctrl_tps[NUM_OVLD_CTRL_SVC][OVLD_MAX_MON_PERIOD]; // �ֱ� mon_period �Ⱓ���� ȣ ���ѵ� count ����
    int     recent_svc_tps[NUM_OVLD_CTRL_SVC]; // �ֱ� mon_period �Ⱓ���� ���TPS. 1�ʸ��� �ֱ� ������ ���ȴ�.
    int     recent_succ_rate[NUM_OVLD_CTRL_SVC]; // �ֱ� mon_period �Ⱓ���� ������. 1�ʸ��� �ֱ� ������ ���ȴ�.
    time_t  last_occur_time; // ������ �߻� �ð�
    time_t  last_clear_time; // ������ ���� �ð�

	/* for mem */
	time_t  mem_last_occur_time; // ������ �߻� �ð�
	time_t  mem_last_clear_time; // ������ ���� �ð�

    time_t  last_monitor_time;  // ������ ���� ���μ����� ���������� ����� �ð�.
                                // ������ ���� ���μ����� �׾� �ִ� ��� curr_tps�� reset ���� �ʾƼ� �������ϰ� �ǹǷ�
                                // ȣ ���� ���� �Ǵ� �� ����ð��� duration �ð� �̻� ���� ���� ������ ���� ó���Ѵ�.
} T_OverloadStatus;

typedef struct {
    T_OverloadConfig    cfg;
    T_OverloadStatus    sts;
    T_OverloadCurrTPS   curr0; // �� �����ð��� Ȧ,¦���� ������ ����Ѵ�. ȣó�� ��Ͽ��� count�ϰ� olcd ��Ͽ��� �����Ҷ� read/write �и�
    T_OverloadCurrTPS   curr1; // ¦�� �ʿ��� curr0��, Ȧ�� �ʿ��� curr1�� ����Ѵ�.
} T_OverloadInfo;

extern char ovld_svc_list[NUM_OVLD_CTRL_SVC][16];

extern int ovldlib_init (char *myAppName);
extern int ovldlib_attach_shm (void);
extern int ovldlib_isOvldCtrl (time_t now, int mode, int svcId, char *operName);
extern int ovldlib_increaseFailCnt (int svcId);


#endif //__OVERLOADLIB_H__
