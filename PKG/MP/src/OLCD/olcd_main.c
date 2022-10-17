#include "olcd_proto.h"

int		olcdQid, ixpcQid, ovldMonLog, statCollectCnt;
char	mySysName[COMM_MAX_NAME_LEN], myAppName[COMM_MAX_NAME_LEN];
OLCD_TpsStatInfo	tpsSTAT[NUM_OVLD_CTRL_SVC];
SFM_SysCommMsgType		*ptr_sadb;
extern T_OverloadInfo	*ovldInfo;
char    standbySysName[COMM_MAX_NAME_LEN];
int	sysMode;
int curSysMode;


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
int main (int ac, char *av[])
{
	char	msgBuff[1024*64];
	struct timeval  now, prev;
	int		i=0;
	int sndSleepCnt = 5;

	if (olcd_initial() < 0) {
		fprintf(stderr,">>>>>> olcd_initial fail\n");
		return -1;
	}

    //GetBlockVersionCompileTime(__DATE__, __TIME__);

	// clear previous queue messages
	while (msgrcv(olcdQid, msgBuff, sizeof(msgBuff), 0, IPC_NOWAIT) > 0);

	gettimeofday (&now, NULL);
	prev.tv_sec = now.tv_sec;

	for (i=0; i<NUM_OVLD_CTRL_SVC; i++) {
		if ( strlen(ovldInfo->cfg.svc_name[i]) < 1 ) {
			continue;
		}
	
		ERRLOG(LLE, FL,"[%s] name=[%s] normal=(%d), ovld=(%d)\n",
				__func__, ovldInfo->cfg.svc_name[i], ovldInfo->cfg.tps_limit_normal[i], ovldInfo->cfg.tps_limit_ovld[i]);
	} /* end of for */

	ERRLOG(LLE, FL,"%s startup...\n", myAppName);

	while (1)
	{
		commlib_microSleep (500);
		curSysMode = sys_ha_get_mode();

		olcd_rcvMsgQ (msgBuff);

		// 2019-1108 : 1초단위, 그리고 매초 0.1초 이후
		// 호 처리 블록이 current0와 current1 위치를 확실하게 옮겨갈 시간을 확보하기 위해 매초 100ms까지 확인한다.
		gettimeofday (&now, NULL);
		if (now.tv_sec == prev.tv_sec || now.tv_usec < 100000)
			continue;
		prev.tv_sec = now.tv_sec;
		keepalivelib_increase ();

		// 서비스별, Appl별 current TPS를 취합하여 1초전(last) TPS에 기록하고 current TPS는 reset한다.
		// mon_period 기간동안 보관 영역에도 저장한다.
		olcd_arrangeCurrTPS (now.tv_sec);

		// 최근 mon_period 기간동안의 tps와 fail count로 평균 success rate을 계산한다.
		// 1초마다 최근 mon_period 기간동안의 success rate을 계산한다.
		olcd_calcuSuccRate ();

		// 서비스별 TPS와 Success Rate을 OMP-FIMD로 전달한다.
		// 로그파일에 TPS와 Success Rate을 기록한다.
		olcd_reportTpsInfo (&sndSleepCnt);

		// 서비스별 TPS와 Success Rate으로 통계를 수집한다.
		olcd_collectTpsStat ();

		// sadb를 참고하여 CPU 과부하 상태를 확인한다. CPU Level을 설정한다.
		// CPU Level 변경 시 OMP-COND로 알람 메시지를 보낸다.
		olcd_checkCpuOverload (now.tv_sec);

		// sadb를 참고하여 MEM 과부하 상태를 확인한다. MEM Level을 설정한다.
		// MEM Level 변경 시 OMP-COND로 알람 메시지를 보낸다.
		olcd_checkMemOverload (now.tv_sec);

		// mon_period 기간동안 누적된 호 제한 건수가 있으면 알람 메시지를 만들어 OMP-COND로 전송한다.
		// 알람메시지를 로그파일에 기록한다.
		if (now.tv_sec % ovldInfo->cfg.mon_period == 0) {
			olcd_checkCallCtrlAlarm (now.tv_sec);
		}
		sysMode = curSysMode;

	}

}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
int olcd_initial (void)
{
	char	fname[256],tmp[64];
	int		key, id;

	strcpy (myAppName, "OLCD");

	if (conflib_initConfigData() < 0) {
		fprintf(stderr, "[%s.%d] conflib_initConfigData() Error\n", FL);
		return -1;
	}

	commlib_setupSignals (NULL);

	if (keepalivelib_init (myAppName) < 0)
		return -1;

	sprintf(fname, "%s/%s", homeEnv, SYSCONF_FILE);
	fprintf(stderr,"[%s] fname=[%s]\n", __func__, fname);
	if (conflib_getNthTokenInFileSection (fname, "APPLICATIONS", myAppName, 3, tmp) < 0) {
		fprintf(stderr,"[%s] APPLICATIONS fail\n", __func__);
		return -1;
	}

	key = strtol(tmp,0,0);
	if ((olcdQid = msgget(key,IPC_CREAT|0666)) < 0) {
		fprintf(stderr,"[%s] msgget fail; key=0x%x,err=%d(%s)\n", __func__,key,errno,strerror(errno));
		return -1;
	}
	if (conflib_getNthTokenInFileSection (fname, "APPLICATIONS", "IXPC", 3, tmp) < 0)
		return -1;
	key = strtol(tmp,0,0);
	if ((ixpcQid = msgget(key,IPC_CREAT|0666)) < 0) {
		fprintf(stderr,"[%s] msgget fail; key=0x%x,err=%d(%s)\n", __func__,key,errno,strerror(errno));
		return -1;
	}

	if (initLog(myAppName) < 0) {
		fprintf(stderr, "[%s.%d] initLog() Error\n", FL);
		return -1;
	}

	if (initConf() < 0) {
		fprintf(stderr, "[%s.%d] initConf() Error\n", FL);
		return -1;
	}

	//---------------------------------------------------------------

	if (ovldlib_init (myAppName) < 0) {
		fprintf(stderr,"[%s] ovldlib_init fail...\n", __func__);
		return -1;
	}
	if (olcd_loadOlvdCfg () < 0) {
		fprintf(stderr,"[%s] olcd_loadOlvdCfg fail...\n", __func__);
		return -1;
	}

	memset (&ovldInfo->curr0, 0, sizeof(T_OverloadCurrTPS));  // clear current_tps
	memset (&ovldInfo->curr1, 0, sizeof(T_OverloadCurrTPS));  // clear current_tps
	memset (ovldInfo->sts.period_svc_tps,  0xff, sizeof(ovldInfo->sts.period_svc_tps));  // clear last period
	memset (ovldInfo->sts.period_fail_tps, 0xff, sizeof(ovldInfo->sts.period_fail_tps)); // clear last period
	memset (ovldInfo->sts.period_ctrl_tps, 0xff, sizeof(ovldInfo->sts.period_ctrl_tps)); // clear last period

	olcd_resetTpsStat (); // 통계값 초기화

    sprintf (fname, "%s/%s", getenv(IV_HOME), SYSCONF_FILE);
	if (conflib_getNthTokenInFileSection (fname, "SHARED_MEMORY_KEY", "SHM_LOC_SADB", 1, tmp) < 0) {
		fprintf (stderr,"[%s] not found SHM_LOC_SADB in %s\n",__func__, fname);
		return -1;
	}
	key = (int)strtol(tmp,0,0);
	if ((id = (int)shmget (key, sizeof(SFM_SysCommMsgType), 0644|IPC_CREAT)) < 0) {
		if (errno != ENOENT) {
			fprintf (stderr,"[%s] shmget fail; key=0x%x, err=%d(%s)\n",__func__, key, errno, strerror(errno));
			return -1;
		}
	}
	ptr_sadb = (SFM_SysCommMsgType *) shmat(id,0,0);
	if ((void*)ptr_sadb == (void *)SHM_FAILED) {
		fprintf (stderr,"[%s] shmat fail; key=0x%x, err=%d(%s)\n", __func__, key, errno, strerror(errno));
		return -1;
	}

	if(sys_init_shm (NULL) == NULL) {
		fprintf(stderr, "[%s.%d] sys_init_shm () ERROR(%d:%s)]\n", FL, errno, strerror(errno));
		return -1;
	}
	sysMode = sys_ha_get_mode();
	curSysMode = sysMode;
	return 1;
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
int olcd_rcvMsgQ (char *buff)
{
    int     i, rcvCnt=0;
	GeneralQMsgType *genQMsg = (GeneralQMsgType *)buff;
//    TrcLibSetPrintMsgType   *trcMsg;

    for (i=rcvCnt=0; i < 10; i++)
    {
        if (msgrcv(olcdQid, genQMsg, sizeof(GeneralQMsgType), 0, IPC_NOWAIT) < 0) {
            if (errno != ENOMSG) {
                ERRLOG(LLE, FL,"[%s] >>> msgrcv fail; err=%d(%s)\n", __func__, errno, strerror(errno));
                exit(1);
            }
            break;
        }
        rcvCnt++;
        switch (genQMsg->mtype){
            case MTYPE_SETPRINT:
#if 0 /* edited by hangkuk at 2020.05.11 */
				trclib_exeSetPrintMsg ((TrcLibSetPrintMsgType*)genQMsg);
                trcMsg = (TrcLibSetPrintMsgType*)genQMsg;
                if (trcMsg->trcLogFlag.pres) {
					*lOG_FLAG = trcMsg->trcLogFlag.octet;
                    ERRLOG(LLE, FL,"---- log level change (Level=%d)", trcMsg->trcLogFlag.octet);
                }
#else 
				trclib_exeSetPrintMsg ((TrcLibSetPrintMsgType*)genQMsg);
#endif /* edited by hangkuk at 2020.05.11 */
                break;

			case MTYPE_MMC_REQUEST:
				olcd_hdlMMCReqMsg (genQMsg);
				break;

			case MTYPE_STATISTICS_REQUEST:
				ERRLOG(LL4, FL,"recv STATISTICS_REQUEST\n");
				olcd_reportStatistics ((IxpcQMsgType *)genQMsg->body);
				break;

            default:
                ERRLOG(LLE, FL,"[%s] Unknown mtype(%ld)\n", __func__, genQMsg->mtype);
                break;
        }
    }
    return rcvCnt;
}



/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
void olcd_arrangeCurrTPS (time_t now)
{
    int     i, j;
    int     sum_svc=0, sum_fail=0, sum_ctrl=0;
    int     last_tot_svc_cnt=0, last_tot_fail_cnt=0, last_tot_ctrl_cnt=0;
	T_OverloadCurrTPS   *currTPS;

    // monitoring 프로세스가 마지막으로 확인시간 기록한다.
    ovldInfo->sts.last_monitor_time = now;

	//--------------------------------------------------------------------------
    // current tps를 reset하고, 1초전 서비스별 tps를 기록한다.
	//--------------------------------------------------------------------------

	// 2019-1108 
	// 호처리 블록에서 count하고 olcd 블록에서 수집할때 read/write 분리하기 위해
	// 초 단위시간을 홀,짝수로 나누어 호처리 블록은 짝수 초에는 curr0에, 홀수 초에는 curr1에 기록하고
	// olcd 블록은 짝수 초에는 curr1를, 호수 초에는 curr0의 정보를 수집하고 reset한다.
	if (now%2==1) currTPS = &ovldInfo->curr0;
	else          currTPS = &ovldInfo->curr1;

    for (i=0; i<NUM_OVLD_CTRL_SVC; i++) {
		if (!strcmp(ovldInfo->cfg.svc_name[i], "")) 
			break;
		sum_svc = sum_fail = sum_ctrl = 0;
        for (j=0; j<NUM_OVLD_CTRL_APPL; j++) {
            sum_svc  += currTPS->svc_tps[i][j];
            sum_fail += currTPS->fail_tps[i][j];
            sum_ctrl += currTPS->ctrl_tps[i][j];
        }
        last_tot_svc_cnt  += sum_svc;  // 1초전 전체 tps
        last_tot_fail_cnt += sum_fail;
        last_tot_ctrl_cnt += sum_ctrl;
        ovldInfo->sts.last_svc_tps[i]  = sum_svc;  // 1초전 서비스별 tps 기록
        ovldInfo->sts.last_fail_tps[i] = sum_fail;
        ovldInfo->sts.last_ctrl_tps[i] = sum_ctrl;
		// 최근 mon_period 기간동안 서비스별 TPS 보관
		memmove (&ovldInfo->sts.period_svc_tps[i][1],  &ovldInfo->sts.period_svc_tps[i][0],  sizeof(int)*(OVLD_MAX_MON_PERIOD-1));
		memmove (&ovldInfo->sts.period_fail_tps[i][1], &ovldInfo->sts.period_fail_tps[i][0], sizeof(int)*(OVLD_MAX_MON_PERIOD-1));
		memmove (&ovldInfo->sts.period_ctrl_tps[i][1], &ovldInfo->sts.period_ctrl_tps[i][0], sizeof(int)*(OVLD_MAX_MON_PERIOD-1));
		ovldInfo->sts.period_svc_tps[i][0]  = sum_svc;
		ovldInfo->sts.period_fail_tps[i][0] = sum_fail;
		ovldInfo->sts.period_ctrl_tps[i][0] = sum_ctrl;
    }

    memset (currTPS, 0, sizeof(T_OverloadCurrTPS));

	return;
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
void olcd_calcuSuccRate (void)
{
	int		i, j, attempt, fail, succ_rate;

	//--------------------------------------------------------------------------
	// 최근 mon_period 기간동안의 tps와 fail count로 평균 success rate을 계산한다.
	// 1초마다 최근(10초) 평균을 구한다.
	//--------------------------------------------------------------------------

    for (i=0; i<NUM_OVLD_CTRL_SVC; i++) {
		if (!strcmp(ovldInfo->cfg.svc_name[i], "")) 
			break;
		attempt = fail = 0;
        for (j=0; j<ovldInfo->cfg.mon_period; j++) {
			if (ovldInfo->sts.period_svc_tps[i][j] < 0) // 초기 구동시 0xff로 기록해 둔 영역 제외
				continue;
            attempt += ovldInfo->sts.period_svc_tps[i][j];
            fail    += ovldInfo->sts.period_fail_tps[i][j];
        }
//		if (attempt < 1 || fail < 1 || (attempt < fail)) {
		if (attempt < 1 || fail < 1) {
			succ_rate = 100;
		}
		else if	(attempt < fail) {
			succ_rate = 0;
		} else {
			succ_rate = ((attempt-fail) * 1000) / attempt; // 1000분율 수치
			succ_rate = (succ_rate + 5) / 10; // 반욜림 반영해서 100분율 수치로 변환
		}
		ovldInfo->sts.recent_succ_rate[i] = succ_rate;
		ovldInfo->sts.recent_svc_tps[i] = attempt / ovldInfo->cfg.mon_period;
    }

	return;
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
void olcd_reportTpsInfo (int *sndSleepCnt)
{
	GeneralQMsgType     txGenQMsg;
	IxpcQMsgType        *txIxpcMsg;
	SFM_SepcItemMsgType	itemData;
	SFM_SepcItemMsgType *standbyTpsData = NULL;
    char    msg_buf[8192];
	int		i, itemCnt=0, txLen, x=0, last_tot_svc_cnt=0, last_tot_ctrl_cnt=0;
    int     cpu_tot, cpu_avg, cpu_cnt;

	//--------------------------------------------------------------------------
	// 서비스별 TPS와 Success Rate을 OMP-FIMD로 전달한다.
	//--------------------------------------------------------------------------

    for (i=0; i<NUM_OVLD_CTRL_SVC; i++) {
		if (!strcmp(ovldInfo->cfg.svc_name[i], "")) 
			break;
		strcpy (itemData.item_sts[itemCnt].name, "TPS");
		strcpy (itemData.item_sts[itemCnt].sub_name, ovldInfo->cfg.svc_name[i]);
        //itemData.item_sts[itemCnt].usage = ovldInfo->sts.last_svc_tps[i];  // 1초전 서비스별 tps 기록
        itemData.item_sts[itemCnt].usage = ovldInfo->sts.recent_svc_tps[i];  // 최근  서비스별 평균 tps 기록
		//fprintf(stderr,"itemData.item_sts[%d].sub_name:%s\n",itemCnt,itemData.item_sts[itemCnt].sub_name);
		//fprintf(stderr,"itemData.item_sts[%d].usage:%d\n",itemCnt,itemData.item_sts[itemCnt].usage);
		itemCnt++;
	}
    for (i=0; i<NUM_OVLD_CTRL_SVC; i++) {
		if (!strcmp(ovldInfo->cfg.svc_name[i], "")) 
			break;
		strcpy (itemData.item_sts[itemCnt].name, "SUCCESS_RATE");
		strcpy (itemData.item_sts[itemCnt].sub_name, ovldInfo->cfg.svc_name[i]);
        itemData.item_sts[itemCnt].usage = ovldInfo->sts.recent_succ_rate[i];  // 최근 서비스별 success rate 기록
		itemCnt++;
	}
	itemData.itemCnt = itemCnt;

	txIxpcMsg = (IxpcQMsgType*)txGenQMsg.body;
	memset ((void*)&txIxpcMsg->head, 0, sizeof(txIxpcMsg->head));
	txGenQMsg.mtype = MTYPE_STATUS_REPORT;
	if(curSysMode != sysMode && curSysMode == MODE_ACTIVE) {
		strcpy (txIxpcMsg->head.srcSysName, standbySysName);
		strcpy (txIxpcMsg->head.srcAppName, myAppName);
		strcpy (txIxpcMsg->head.dstSysName, "OMP");
		strcpy (txIxpcMsg->head.dstAppName, "FIMD");

		txIxpcMsg->head.msgId   = MSGID_SYS_SPEC_ITEM_DATA;
		txIxpcMsg->head.bodyLen = sizeof(SFM_SepcItemMsgType);
		txLen = sizeof(txIxpcMsg->head) + txIxpcMsg->head.bodyLen;
		memset(txIxpcMsg->body, 0x00, sizeof(SFM_SepcItemMsgType));
		standbyTpsData = (SFM_SepcItemMsgType*)txIxpcMsg->body;
		itemCnt = 0;
		for (i=0; i<NUM_OVLD_CTRL_SVC; i++) {
			if (!strcmp(ovldInfo->cfg.svc_name[i], ""))
				break;
			strcpy (standbyTpsData->item_sts[itemCnt].name, "TPS");
			strcpy (standbyTpsData->item_sts[itemCnt].sub_name, ovldInfo->cfg.svc_name[i]);
			standbyTpsData->item_sts[itemCnt].usage = 0;
			itemCnt++;
		}
		standbyTpsData->itemCnt = itemCnt;	
		if (msgsnd (ixpcQid, (void*)&txGenQMsg, txLen, IPC_NOWAIT) < 0) {
			ERRLOG(LLE, FL,"[%s] >>> msgsnd fail; err=%d(%s)", __func__, errno, strerror(errno));
		}
		ERRLOG(LLE, FL,"[%s] >>> msgsnd standbySysName=%s curSysMode=%d sysMode=%d ", __func__, standbySysName, curSysMode, sysMode);
	}
	strcpy (txIxpcMsg->head.srcSysName, mySysName);
	strcpy (txIxpcMsg->head.srcAppName, myAppName);
	strcpy (txIxpcMsg->head.dstSysName, "OMP");
	strcpy (txIxpcMsg->head.dstAppName, "FIMD");

	txIxpcMsg->head.msgId   = MSGID_SYS_SPEC_ITEM_DATA;
	txIxpcMsg->head.bodyLen = sizeof(SFM_SepcItemMsgType);
	txLen = sizeof(txIxpcMsg->head) + txIxpcMsg->head.bodyLen;

	memcpy ((void*)txIxpcMsg->body, &itemData, sizeof(SFM_SepcItemMsgType));

	if(!(*sndSleepCnt)) {
		if (msgsnd (ixpcQid, (void*)&txGenQMsg, txLen, IPC_NOWAIT) < 0) {
			ERRLOG(LLE, FL,"[%s] >>> msgsnd fail; err=%d(%s)", __func__, errno, strerror(errno));
		}
	}
	else {
		(*sndSleepCnt)--;
	}

	//--------------------------------------------------------------------------
	// 1초간 서비스별 TPS와 SuccRate을 로그파일에 기록한다.
	//--------------------------------------------------------------------------

    cpu_cnt = ptr_sadb->cpuCount;
	if (cpu_cnt < 1) cpu_cnt = 1;

    for (i=cpu_tot=0; i < ptr_sadb->cpuCount; i++)
		cpu_tot += ptr_sadb->loc_cpu_sts[i].cpu_usage;
    cpu_avg = cpu_tot / cpu_cnt; // 1000분율 값이다.

	x = 0;
    x += sprintf (&msg_buf[x], "cpu=[%2d.%d] ", cpu_avg/10, cpu_avg%10);
	x += sprintf (&msg_buf[x], "mem=[%2d.%d] ", ptr_sadb->loc_mem_sts.mem_usage/10, ptr_sadb->loc_mem_sts.mem_usage%10);
    x += sprintf (&msg_buf[x], "tps=[tot");
    for (i=0; i<NUM_OVLD_CTRL_SVC; i++) {
		if (!strcmp(ovldInfo->cfg.svc_name[i], "")) 
			break;
		x += sprintf (&msg_buf[x], ":%s", ovldInfo->cfg.svc_name[i]);
        last_tot_svc_cnt  += ovldInfo->sts.last_svc_tps[i];
        last_tot_ctrl_cnt += ovldInfo->sts.last_ctrl_tps[i];
	}
	x += sprintf (&msg_buf[x], "]=[%d", last_tot_svc_cnt);
    for (i=0; i<NUM_OVLD_CTRL_SVC; i++) {
		if (!strcmp(ovldInfo->cfg.svc_name[i], "")) 
			break;
		x += sprintf (&msg_buf[x], ":%d", ovldInfo->sts.last_svc_tps[i]);
	}
	x += sprintf (&msg_buf[x], "] rate=[%d", ovldInfo->sts.recent_succ_rate[0]);
    for (i=1; i<NUM_OVLD_CTRL_SVC; i++) {
		if (!strcmp(ovldInfo->cfg.svc_name[i], "")) 
			break;
		x += sprintf (&msg_buf[x], ":%d", ovldInfo->sts.recent_succ_rate[i]);
	}
	x += sprintf (&msg_buf[x], "]");

    if (last_tot_ctrl_cnt > 0) {
        x += sprintf (&msg_buf[x], " ctrl=[%d", last_tot_ctrl_cnt);
        for (i=0; i<NUM_OVLD_CTRL_SVC; i++) {
            if (!strcmp(ovldInfo->cfg.svc_name[i], "")) 
                break;
            x += sprintf (&msg_buf[x], ":%d", ovldInfo->sts.last_ctrl_tps[i]);
        }
        x += sprintf (&msg_buf[x], "]");
    }

	logPrint(ovldMonLog, FL, "%s\n", msg_buf);

	return;
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
void olcd_collectTpsStat (void)
{
	int		i;

    for (i=0; i<NUM_OVLD_CTRL_SVC; i++) {
		if (!strcmp(ovldInfo->cfg.svc_name[i], "")) 
			break;
        tpsSTAT[i].sum_tps       += ovldInfo->sts.last_svc_tps[i];
        tpsSTAT[i].sum_succ_rate += ovldInfo->sts.recent_succ_rate[i];
		if (tpsSTAT[i].max_tps < ovldInfo->sts.last_svc_tps[i])
			tpsSTAT[i].max_tps = ovldInfo->sts.last_svc_tps[i];
        if (tpsSTAT[i].min_succ_rate > ovldInfo->sts.recent_succ_rate[i])
			tpsSTAT[i].min_succ_rate = ovldInfo->sts.recent_succ_rate[i];
	}
	statCollectCnt++;

	return;
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
void olcd_resetTpsStat (void)
{
	int		i;

	statCollectCnt = 0;

    for (i=0; i<NUM_OVLD_CTRL_SVC; i++) {
		if (!strcmp(ovldInfo->cfg.svc_name[i], "")) 
			break;
		strcpy (tpsSTAT[i].type, ovldInfo->cfg.svc_name[i]);
        tpsSTAT[i].sum_tps       = 0;
        tpsSTAT[i].avg_tps       = 0;
		tpsSTAT[i].max_tps       = 0;
        tpsSTAT[i].sum_succ_rate = 0;
        tpsSTAT[i].avg_succ_rate = 0;
		tpsSTAT[i].min_succ_rate = 100;
	}

	return;
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
void olcd_reportStatistics (IxpcQMsgType *rxIxpcMsg)
{
	GeneralQMsgType         genQMsg;
	IxpcQMsgType            *txIxpcMsg = (IxpcQMsgType *)genQMsg.body;
	STM_CommonStatMsgType   *commStat = (STM_CommonStatMsgType *)txIxpcMsg->body;
	int     i, x=0, tenVal, txLen;
	char	tmpbuf[1024*32];

	if (statCollectCnt < 1) {
		ERRLOG(LL4, FL,"(%s) ignore... statCollectCnt=%d\n", __func__, statCollectCnt);
		return;
	}

	// average 값 계산
	//
    for (i=0; i<NUM_OVLD_CTRL_SVC; i++) {
		if (!strcmp(ovldInfo->cfg.svc_name[i], "")) 
			break;
        tenVal = (tpsSTAT[i].sum_tps * 10) / statCollectCnt; // 소수점 1자리 반올림 처리 위해 10배 수치로 구한다.
        tpsSTAT[i].avg_tps = (tenVal + 5) / 10; // 반올림 처리한 수치로 바꾼다.
        tenVal = (tpsSTAT[i].sum_succ_rate * 10) / statCollectCnt;
        tpsSTAT[i].avg_succ_rate = (tenVal + 5) / 10;
	}

	memset (&genQMsg, 0, sizeof(GeneralQMsgType));

	genQMsg.mtype = MTYPE_STATISTICS_REPORT;
	txIxpcMsg->head.msgId = MSGID_TPS_STATISTICS_REPORT;
	strcpy (txIxpcMsg->head.srcSysName, mySysName);
	strcpy (txIxpcMsg->head.srcAppName, myAppName);
	strcpy (txIxpcMsg->head.dstSysName, rxIxpcMsg->head.srcSysName);
	strcpy (txIxpcMsg->head.dstAppName, rxIxpcMsg->head.srcAppName);

    for (i=0; i<NUM_OVLD_CTRL_SVC; i++) {
		if (!strcmp(ovldInfo->cfg.svc_name[i], "")) 
			break;
		strcpy (commStat->info[i].strkey1, tpsSTAT[i].type);
		commStat->info[i].ldata[0] = tpsSTAT[i].avg_tps;
		commStat->info[i].ldata[1] = tpsSTAT[i].max_tps;
		commStat->info[i].ldata[2] = tpsSTAT[i].avg_succ_rate;
		commStat->info[i].ldata[3] = tpsSTAT[i].min_succ_rate;
		x += sprintf (&tmpbuf[x], "\n  strKey=%s avg_tps=%ld max_tps=%ld avg_succ_rate=%ld min_succ_rate=%ld", commStat->info[i].strkey1,
				commStat->info[i].ldata[0], commStat->info[i].ldata[1], commStat->info[i].ldata[2], commStat->info[i].ldata[3]);
	}
	commStat->num = i;
	txIxpcMsg->head.bodyLen = sizeof(commStat->num) + (commStat->num * sizeof(STM_CommonStatMsg));
	txLen =  sizeof(genQMsg.mtype) + sizeof(IxpcQMsgHeadType) + txIxpcMsg->head.bodyLen;

	if (msgsnd (ixpcQid, &genQMsg, txLen, IPC_NOWAIT) < 0) {
		ERRLOG(LLE, FL,"(%s) >>> msgsnd() fail; err=%d(%s)\n", __func__, errno, strerror(errno));
	} else {
		ERRLOG(LL4, FL,"(%s) typeCnt=%d txLen=%d %s\n", __func__, commStat->num, txLen, tmpbuf);
	}

	olcd_resetTpsStat (); // 통계값 초기화

	return;
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
void olcd_checkCpuOverload (time_t now)
{
	int		i, curr_cpu_avg, cpu_tot, cpu_cnt;

	//--------------------------------------------------------------------------
	// sadb를 참고하여 CPU 과부하 상태를 확인한다. CPU Level을 설정한다.
	// CPU Level 변경 시 OMP-COND로 알람 메시지를 보낸다.
	//--------------------------------------------------------------------------

    cpu_cnt = ptr_sadb->cpuCount;
	if (cpu_cnt < 1) cpu_cnt = 1;

    for (i=cpu_tot=0; i < ptr_sadb->cpuCount; i++)
		cpu_tot += ptr_sadb->loc_cpu_sts[i].cpu_usage;
    curr_cpu_avg = cpu_tot / cpu_cnt;       // 1000분율 값이다.
	curr_cpu_avg = (curr_cpu_avg + 5) / 10; // 반욜림 반영해서 100분율 수치로 변환

    if (ovldInfo->sts.curr_level == OVLD_CTRL_CLEAR)
    {
        if (curr_cpu_avg < ovldInfo->cfg.cpu_limit_occur) {
            ovldInfo->sts.occur_try = 0;
            return;
        }
        ovldInfo->sts.occur_try++;
		ERRLOG(LLE, FL,"[%s] --- cpu occur_try=[%d]\n", __func__, ovldInfo->sts.occur_try);
        if (ovldInfo->sts.occur_try < ovldInfo->cfg.cpu_duration) {
            return;
        }
        ovldInfo->sts.curr_level = OVLD_CTRL_SET;
        ovldInfo->sts.last_occur_time = now;
        logPrint(ovldMonLog, FL,">>> cpu overload status change... CLEAR -> SET\n");
		ERRLOG(LLE, FL,"[%s] >>> cpu overload status change... CLEAR -> SET\n", __func__);
        olcd_reportOverLoadAlarm(EVENT_ALM_CPU_OVERLOAD, SFM_ALM_CRITICAL, "CPU OVERLOAD", "CPU OVERLOAD OCCURED");
    }
    else // ovldInfo->sts.curr_level == OVLD_CTRL_SET
    {
        if (curr_cpu_avg >= ovldInfo->cfg.cpu_limit_clear) {
            ovldInfo->sts.clear_try = 0;
            return;
        }
        ovldInfo->sts.clear_try++;
        logPrint(ovldMonLog, FL,"--- cpu clear_try=[%d]\n", ovldInfo->sts.clear_try);
		ERRLOG(LLE, FL,"[%s] --- cpu clear_try=[%d]\n", __func__, ovldInfo->sts.clear_try);
        if (ovldInfo->sts.clear_try < ovldInfo->cfg.cpu_duration) {
            return;
        }
        ovldInfo->sts.curr_level = OVLD_CTRL_CLEAR;
        ovldInfo->sts.last_clear_time = now;
        logPrint(ovldMonLog, FL," <<< cpu overload status change... SET -> CLEAR \n");
		ERRLOG(LLE, FL,"[%s] >>> cpu overload status change... SET -> CLEAR\n", __func__);
        olcd_reportOverLoadAlarm(EVENT_ALM_CPU_OVERLOAD, SFM_ALM_NORMAL, "CPU OVERLOAD", "CPU OVERLOAD CLEARED");
    }

	return;
}

/*------------------------------------------------------------------------------
 * ------------------------------------------------------------------------------*/
void olcd_checkMemOverload (time_t now)
{
	int     curr_mem_usage=0;

	//--------------------------------------------------------------------------
	// sadb를 참고하여 MEM 과부하 상태를 확인한다. MEM Level을 설정한다.
	// MEM Level 변경 시 OMP-COND로 알람 메시지를 보낸다.
	//--------------------------------------------------------------------------

	curr_mem_usage = (ptr_sadb->loc_mem_sts.mem_usage + 5) / 10; // 반욜림 반영해서 100분율 수치로 변환

	if (ovldInfo->sts.mem_curr_level == OVLD_CTRL_CLEAR)
	{
		if (curr_mem_usage < ovldInfo->cfg.mem_limit_occur) {
			ovldInfo->sts.mem_occur_try = 0;
			return;
		}

		ovldInfo->sts.mem_occur_try++;
		logPrint (trcMsgLogId,FL, "[%s] --- mem mem_occur_try=[%d]\n", __func__, ovldInfo->sts.mem_occur_try);

		if (ovldInfo->sts.mem_occur_try < ovldInfo->cfg.mem_duration) {
			return;
		}

		ovldInfo->sts.mem_curr_level = OVLD_CTRL_SET;
		ovldInfo->sts.mem_last_occur_time = now;

		logPrint (ovldMonLog,FL,">>> mem overload status change... CLEAR -> SET\n");
		logPrint (trcMsgLogId,FL, "[%s] >>> mem overload status change... CLEAR -> SET\n", __func__);

		olcd_reportOverLoadAlarm(EVENT_ALM_MEM_OVERLOAD, SFM_ALM_CRITICAL, "MEM OVERLOAD", "MEM OVERLOAD OCCURED");
	}
	else // ovldInfo->sts.mem_curr_level == OVLD_CTRL_SET
	{
		if (curr_mem_usage >= ovldInfo->cfg.mem_limit_clear) {
			ovldInfo->sts.mem_clear_try = 0;
			return;
		}

		ovldInfo->sts.mem_clear_try++;
		logPrint (ovldMonLog,FL,"--- mem mem_clear_try=[%d]\n", ovldInfo->sts.mem_clear_try);
		logPrint (trcMsgLogId,FL, "[%s] --- mem mem_clear_try=[%d]\\n", __func__, ovldInfo->sts.mem_clear_try);
		if (ovldInfo->sts.mem_clear_try < ovldInfo->cfg.mem_duration) {
			return;
		}

		ovldInfo->sts.mem_curr_level = OVLD_CTRL_CLEAR;
		ovldInfo->sts.mem_last_clear_time = now;

		logPrint (ovldMonLog,FL," <<< mem overload status change... SET -> CLEAR \n");
		logPrint (trcMsgLogId,FL, "[%s] >>> mem overload status change... SET -> CLEAR\n", __func__);

		olcd_reportOverLoadAlarm(EVENT_ALM_MEM_OVERLOAD, SFM_ALM_NORMAL, "MEM OVERLOAD", "MEM OVERLOAD CLEARED");
	}

	return;
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
void olcd_checkCallCtrlAlarm (time_t now)
{
    char    msg_buf[8192], evtAlmInfo[64], evtAlmDesc[64];
	int		i, j, x=0, attempt[NUM_OVLD_CTRL_SVC], fail[NUM_OVLD_CTRL_SVC], ctrl[NUM_OVLD_CTRL_SVC];
	int		tot_attempt, tot_fail, tot_ctrl;

	//--------------------------------------------------------------------------
	// mon_period 기간동안 누적된 호 제한 건수가 있으면 알람 메시지를 만들어 OMP-COND로 전송한다.
	// 알람메시지를 로그파일에 기록한다.
	//--------------------------------------------------------------------------

	memset (attempt, 0, sizeof(attempt));
	memset (fail, 0, sizeof(fail));
	memset (ctrl, 0, sizeof(ctrl));
	tot_attempt = tot_fail = tot_ctrl = 0;

    for (i=0; i<NUM_OVLD_CTRL_SVC; i++) {
		if (!strcmp(ovldInfo->cfg.svc_name[i], "")) 
			break;
        for (j=0; j<ovldInfo->cfg.mon_period; j++) {
			if (ovldInfo->sts.period_svc_tps[i][j] < 0) // 초기 구동시 0xff로 기록해 둔 영역 제외
				continue;
            attempt[i]  += ovldInfo->sts.period_svc_tps[i][j];
			tot_attempt += ovldInfo->sts.period_svc_tps[i][j];
            fail[i]     += ovldInfo->sts.period_fail_tps[i][j];
			tot_fail    += ovldInfo->sts.period_fail_tps[i][j];
            ctrl[i]     += ovldInfo->sts.period_ctrl_tps[i][j];
            tot_ctrl    += ovldInfo->sts.period_ctrl_tps[i][j];
        }
    }
	if (tot_ctrl < 1)
		return;

	x += sprintf (&msg_buf[x], "  %-11s = %s\n", "SYSTEM", getenv(MY_SYS_NAME));
	x += sprintf (&msg_buf[x], "  %-11s = %s\n", "NOTI_TYPE", "OVERLOAD CONTROL");
	x += sprintf (&msg_buf[x], "  %-11s = %d seconds\n", "DURATION", ovldInfo->cfg.mon_period);
	x += sprintf (&msg_buf[x], "  %-11s = %s\n", "CPU_LEVEL", ovldInfo->sts.curr_level==OVLD_CTRL_SET ? "Overload" : "Normal");
	x += sprintf (&msg_buf[x], "  %-11s = %s\n", "MEM_LEVEL", ovldInfo->sts.mem_curr_level==OVLD_CTRL_SET ? "Overload" : "Normal");
#if 0 /* edited by hangkuk at 2019.07.20 */
	x += sprintf (&msg_buf[x], "      Service : LMT(1sec)  ATT(%dsec)   CTRL_CNT\n", ovldInfo->cfg.mon_period);
#else
	x += sprintf (&msg_buf[x], "    %15s :  LMT(1sec)  ATT(%dsec)  CTRL_CNT\n", "Service", ovldInfo->cfg.mon_period);
#endif /* end of hangkuk at 2019.07.20 */
	for (i=0; i<NUM_OVLD_CTRL_SVC; i++) {
		if (!strcmp(ovldInfo->cfg.svc_name[i], "")) 
			break;
		x += sprintf (&msg_buf[x], "    %15s = %12d:%-d %10d %10d\n", ovldInfo->cfg.svc_name[i],
				ovldInfo->sts.curr_level==OVLD_CTRL_SET ? ovldInfo->cfg.tps_limit_ovld[i] : ovldInfo->cfg.tps_limit_normal[i],
				ovldInfo->sts.mem_curr_level==OVLD_CTRL_SET ? ovldInfo->cfg.tps_limit_ovld[i] : ovldInfo->cfg.tps_limit_normal[i],
				attempt[i], ctrl[i]);
	}
	x += sprintf (&msg_buf[x], "    %-15s = %10s %10d %10d\n", "Total", "-", tot_attempt, tot_ctrl);
	x += sprintf (&msg_buf[x], "COMPLETED\n\n");

	lprintf(ovldMonLog, LLE, FL, "%s\n%s", __func__, msg_buf);

    //----------------------------------------------------------------------------------------------

    // OverLoad 에 대한 정보를 FIMD로 Noti로 보내어 Noti-History및 Console로 출력하도록 한다.
    //
    olcd_sendNotiMsg2FIMD (msg_buf);

    // TPS별호제어 Alarm Event를 보낸다.
    //
    for (i=0; i<NUM_OVLD_CTRL_SVC; i++) {
		if (!strcmp(ovldInfo->cfg.svc_name[i], "")) 
			break;
        if (ctrl[i] <= 0)
            continue;
        sprintf (evtAlmInfo, "TPS OVERLOAD SVCNAME(%s)", ovldInfo->cfg.svc_name[i]);
        sprintf (evtAlmDesc, "CALL CONTROL COUNT(%d)", ctrl[i]);
        olcd_reportOverLoadAlarm(EVENT_ALM_TPS_OVERLOAD, SFM_ALM_CRITICAL, evtAlmInfo, evtAlmDesc);
    }

	return;
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
void olcd_sendNotiMsg2FIMD (char *msg)
{
    GeneralQMsgType txGenQMsg;
    IxpcQMsgType    *txIxpcMsg;
    int     txLen;

    txGenQMsg.mtype = MTYPE_NOTI_REPORT;

    txIxpcMsg = (IxpcQMsgType*)txGenQMsg.body;
    memset ((void*)&txIxpcMsg->head, 0, sizeof(txIxpcMsg->head));

    strcpy (txIxpcMsg->head.srcSysName, mySysName);
    strcpy (txIxpcMsg->head.srcAppName, myAppName);
    strcpy (txIxpcMsg->head.dstSysName, "OMP");
    strcpy (txIxpcMsg->head.dstAppName, "FIMD");
    txIxpcMsg->head.msgId   = STSCODE_OVERLOAD_CONTROL; // NMSIB에서만 임시로 사용하는 값이다. COND에서는 사용하지 않는

    strcpy (txIxpcMsg->body, msg);
    txIxpcMsg->head.bodyLen = strlen(msg);
    txLen = sizeof(txIxpcMsg->head) + txIxpcMsg->head.bodyLen;

	ERRLOG(LLE, FL,"[%s] tx Call Control Notification;\n%s", __func__, msg);

    if (msgsnd(ixpcQid, (void*)&txGenQMsg, txLen, IPC_NOWAIT) < 0) {
		ERRLOG(LLE, FL,"[%s] >>> msgsnd fail; err=%d(%s)", __func__, errno, strerror(errno));
    }
    return;
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
void olcd_reportOverLoadAlarm(int code, int level, char *info, char *desc)
{   
    GeneralQMsgType     txGenQMsg;
    IxpcQMsgType        *txIxpcMsg;
    AlmMsgInfo          *almMsg;
    int                 txLen;
    
    txIxpcMsg = (IxpcQMsgType*)txGenQMsg.body;
    almMsg = (AlmMsgInfo*)txIxpcMsg->body;

    memset((void*)&txIxpcMsg->head, 0, sizeof(txIxpcMsg->head));
    memset(almMsg, 0x00, sizeof(AlmMsgInfo));
    
    txGenQMsg.mtype = MTYPE_ALARM_REPORT;
    
    strcpy (txIxpcMsg->head.srcSysName, mySysName);
    strcpy (txIxpcMsg->head.srcAppName, myAppName);
    strcpy (txIxpcMsg->head.dstSysName, "OMP");
    strcpy (txIxpcMsg->head.dstAppName, "FIMD");
    txIxpcMsg->head.msgId   = code;
    txIxpcMsg->head.bodyLen = sizeof(AlmMsgInfo);
    
    txLen = sizeof(txIxpcMsg->head) + txIxpcMsg->head.bodyLen;
    
    almMsg->almCode = code;
    almMsg->almLevel = level;
    sprintf(almMsg->almInfo, info);
    sprintf(almMsg->almDesc, desc);
    
	ERRLOG(LLE, FL,"[%s] tx Overload Event Alarm; almCode=%d level=%d almInfo=(%s) almDesc=(%s)", __func__, code, level, info, desc);

    if (msgsnd(ixpcQid, (void*)&txGenQMsg, txLen, IPC_NOWAIT) < 0) {
		ERRLOG(LLE, FL,"[%s] >>> msgsnd fail; err=%d(%s)", __func__, errno, strerror(errno));
    }
    return;
}

/* add by hangkuk at 2019.07.19 */
int olcd_makeDirPath(char *dirPath) 
{
	DIR         *dp;
	char        fullPath[_MAX_PATH];
	char        *sp;
	int         nLen;

   if ( dirPath == NULL ) {
	   fprintf(stderr, "[%s] Error. dirPath is Null\n", __func__);
	   return -1;
   }

   nLen = strlen(dirPath);

   if ( nLen >= _MAX_PATH ) {
	   fprintf(stderr, "[%s] Error. Input Path Length is too long, len=(%d)\n" , __func__, nLen);
	   return -1;
   }

   memset(fullPath, 0x00, sizeof(fullPath));

   strcpy(fullPath, dirPath);
   sp = fullPath;

   while( (sp = strchr(sp, '/')) )
   {
	   if ( sp > fullPath && *(sp - 1) != ':' ) {
		   *sp = '\0';
		   if ( (dp=opendir(fullPath)) == NULL ) {
			   mkdir(fullPath, 0755);
		   }
		   else {
			   closedir(dp);
		   }

		   *sp = '/';
	   }
	   sp++;
   } /* end of for */

	return 1;
}

