#ifndef __OLCD_PROTO_H__
#define __OLCD_PROTO_H__

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/msg.h>

#include <commlib.h>
#include <comm_msgtypes.h>
#include <overloadlib.h>
#include <stm_msgtypes.h>

typedef struct {
	char    type[COMM_MAX_NAME_LEN];
	long    sum_tps;
	long    avg_tps;
	long    max_tps;
	long    sum_succ_rate;
	long    avg_succ_rate;
	long    min_succ_rate;
} OLCD_TpsStatInfo;

extern int	errno;
extern int     olcdQid, ixpcQid, ovldMonLog, statCollectCnt;

/* olcd_main.c */
extern int main(int ac, char *av[]);
extern int olcd_initial (void);
extern int olcd_rcvMsgQ (char *buff);
extern void olcd_arrangeCurrTPS (time_t now);
extern void olcd_calcuSuccRate (void);
extern void olcd_reportTpsInfo(int *sndSleepCnt);
extern void olcd_collectTpsStat (void);
extern void olcd_resetTpsStat (void);
extern void olcd_reportStatistics (IxpcQMsgType *rxIxpcMsg);
extern void olcd_checkCpuOverload (time_t now);
extern void olcd_checkCallCtrlAlarm (time_t now);
extern void olcd_sendNotiMsg2FIMD (char *msg);
extern void olcd_reportOverLoadAlarm(int code, int level, char *info, char *desc);
extern void olcd_checkMemOverload (time_t now);

/* olcd_cfg.c */
extern int initConf();
extern int loadAppConf(char *resBuf);
extern void printAppConf(char *resBuf);
extern int olcd_loadOlvdCfg (void);
extern int olcd_bkupOlvdCfg (void);
extern int olcd_hdlMMCReqMsg (GeneralQMsgType *genQMsg);
extern int olcd_dis_ovld_info (IxpcQMsgType *rxIxpcMsg);
extern int olcd_chg_ovld_cfg (IxpcQMsgType *rxIxpcMsg);
extern int olcd_reload_ovld_cfg (IxpcQMsgType *rxIxpcMsg);
extern int olcd_sendMMCResponse (IxpcQMsgType *rxIxpcMsg, char *resBuf, int resCode);
extern int olcd_makeDirPath(char *dirPath);

/* olcd_log.c */
extern int initLog(char *appName);

#endif /*__OLCD_PROTO_H__*/
