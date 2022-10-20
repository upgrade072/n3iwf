#include <dbm.h>

int dis_msg_trc(IxpcQMsgType *rxIxpcMsg)
{
	int		i;
    char    mmlBuf[8109], *p = mmlBuf;
	char	str[12];

	p += mmclib_printSuccessHeader(p);

    p += sprintf(p, "  ----------------------------------------------------------------\n");
    p += sprintf(p, "  TYPE   TRACE NUM               EXPIRED_TIME (REMAIN_TIME) \n");
    p += sprintf(p, "  ----------------------------------------------------------------\n");

	for (i = 0; i < MAX_TRACE_LIST; i++) {
		if (!trcList->info[i].trcNum[0]) 
			continue;

		memset(str,0x00,sizeof(str));
		sprintf(str,"%s","UE_ID");

		p += sprintf(p, "  %-6s %-17s    %-14s (%d sec)\n"
					  , str,trcList->info[i].trcNum, commlib_printDateTime(trcList->info[i].expiredTime) 
					  , (int)trcList->info[i].expiredTime - (int)time(NULL));
	}

	if (!trcList->listCnt)
		p += sprintf(p, "\n");

    p += sprintf(p, "  ----------------------------------------------------------------\n");
	p += sprintf(p, "  TRACE TOTAL = %d\n", trcList->listCnt);
	mmclib_sendMmcResult(rxIxpcMsg, mmlBuf, RES_SUCCESS, FLAG_COMPLETE, 0, 0, 0);

    return 0; 
}

int reg_msg_trc(IxpcQMsgType *rxIxpcMsg)
{
	int		nRet;
    char    mmlBuf[4096], *p=mmlBuf, reason[4096];
	char	value[MAX_TRC_LEN+1];
	time_t	expiredTime;

	memset(value, 0, sizeof(value));

    if (mmclib_getMmlPara_STR(rxIxpcMsg, "UE_ID", value) < 0) {
        mmclib_printFailureHeader(mmlBuf, "MANDATORY PARAMETER(UE_ID) MISSING");
        mmclib_sendMmcResult(rxIxpcMsg, mmlBuf, RES_FAIL, FLAG_COMPLETE, 0, 0, 0);
        return -1;
	}

	expiredTime = mmclib_getMmlPara_INT(rxIxpcMsg, "PERIOD");

	nRet = mmclib_getMmlPara_INT(rxIxpcMsg, "PERIOD");
	if (nRet < 0) {
		expiredTime = time(0) + 60 * 2880;
	} else {
		expiredTime = nRet*60 + time(0);
	}

	nRet = msgtrclib_addTraceList(value, expiredTime, reason, 0, 1 /* trace level */);
	if (nRet < 0) {
		mmclib_printFailureHeader(mmlBuf, reason);
		mmclib_sendMmcResult(rxIxpcMsg, mmlBuf, RES_FAIL, FLAG_COMPLETE, 0, 0, 0);
		return -1;
	}

	p += mmclib_printSuccessHeader(p);
	p += sprintf(p, "  UE_ID        = %s\n", value);
	p += sprintf(p, "  EXPIRED TIME = %s\n", commlib_printDateTime(expiredTime));
	p += sprintf(p, "  TRACE COUNT  = %d\n", trcList->listCnt);

	mmclib_sendMmcResult(rxIxpcMsg, mmlBuf, RES_SUCCESS, FLAG_COMPLETE, 0, 0, 0);

    return 0;
}


int canc_msg_trc(IxpcQMsgType *rxIxpcMsg)
{
	int		nRet;
    char    mmlBuf[4096], *p=mmlBuf, reason[4096];
	char	value[MAX_TRC_LEN+1];

	memset(value,  0, sizeof(value));

    if (mmclib_getMmlPara_STR(rxIxpcMsg, "UE_ID",  value) < 0) {
        mmclib_printFailureHeader(mmlBuf, "MANDATORY PARAMETER(MAC) MISSING");
        mmclib_sendMmcResult(rxIxpcMsg, mmlBuf, RES_FAIL, FLAG_COMPLETE, 0, 0, 0);
        return -1;
	}

	nRet = msgtrclib_delTraceList(value, reason);
	if (nRet < 0) {
		mmclib_printFailureHeader(mmlBuf, reason);
		mmclib_sendMmcResult(rxIxpcMsg, mmlBuf, RES_FAIL, FLAG_COMPLETE, 0, 0, 0);
		return -1;
	}

	p += mmclib_printSuccessHeader(p);
	if (value[0])
		p += sprintf(p, "  VALUE     = %s\n", value);
	
	p += sprintf(p,  "  TRACE DELETED \n");

	mmclib_sendMmcResult(rxIxpcMsg, mmlBuf, RES_SUCCESS, FLAG_COMPLETE, 0, 0, 0);

	return 0;
}

