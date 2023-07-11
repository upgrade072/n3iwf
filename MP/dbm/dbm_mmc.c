#include <dbm.h>

int numMmcHdlr;

MmcHdlrVector   mmcHdlrVector[4096] =
{
	{ "reload-config-data",		mmib_relay_mmc},
	{ "dis-config-data",		mmib_relay_mmc},

    { "dis-trc-info",			dis_msg_trc },
    { "add-trc-info",			reg_msg_trc },
    { "del-trc-info",			canc_msg_trc },

	{"", NULL}
};

int initMmc()
{
    int i;
    for(i = 0; i < 4096; i++) {
        if(mmcHdlrVector[i].func == NULL)
            break;
    }
    numMmcHdlr = i;

    qsort ( (void*)mmcHdlrVector,
                   numMmcHdlr,
                   sizeof(MmcHdlrVector),
                   mmcHdlrVector_qsortCmp );
    return 1;
}

void handleMMCReq (IxpcQMsgType *rxIxpcMsg)
{
    MMLReqMsgType       *mmlReqMsg;
    MmcHdlrVector       *mmcHdlr;

    mmlReqMsg = (MMLReqMsgType*)rxIxpcMsg->body;

    if ((mmcHdlr = (MmcHdlrVector*) bsearch (
                    mmlReqMsg->head.cmdName,
                    mmcHdlrVector,
                    numMmcHdlr,
                    sizeof(MmcHdlrVector),
                    mmcHdlrVector_bsrchCmp)) == NULL) {
        logPrint(ELI, FL, "handleMMCReq() received unknown mml_cmd(%s)\n", mmlReqMsg->head.cmdName);
        return ;
    }
 	logPrint(LL3, FL, "handleMMCReq() received mml_cmd(%s)\n", mmlReqMsg->head.cmdName);

    (int)(*(mmcHdlr->func)) (rxIxpcMsg);

	return;
}

int mmcHdlrVector_qsortCmp (const void *a, const void *b)
{
    return (strcasecmp (((MmcHdlrVector*)a)->cmdName, ((MmcHdlrVector*)b)->cmdName));
}

int mmcHdlrVector_bsrchCmp (const void *a, const void *b)
{
	return (strcasecmp ((char*)a, ((MmcHdlrVector*)b)->cmdName));
}

int mmcReloadConfigData(IxpcQMsgType *rxIxpcMsg)
{
	int 	len;
    char    resBuf[4096];
    char    mmlBuf[4096];

	len = 0;
	if(loadAppConfig(resBuf) < 0) {
        len += sprintf(&mmlBuf[len], "\n  RESULT = FAIL\n  SYSTEM = %s\n\n", mySysName);
		len += sprintf(&mmlBuf[len], "  FAIL REASON =  %s\n", resBuf);
		mmclib_sendMmcResult(rxIxpcMsg, mmlBuf, RES_FAIL, FLAG_COMPLETE, 0, 0, 0);
        return -1;
	}

    len += sprintf(&mmlBuf[len], "\n  RESULT = SUCCESS\n  SYSTEM = %s\n\n", mySysName);
    len += sprintf(&mmlBuf[len], "%s\n\n", resBuf);
	mmclib_sendMmcResult(rxIxpcMsg, mmlBuf, RES_SUCCESS, FLAG_COMPLETE, 0, 0, 0);

    return 0;
}

int mmcDisConfigData(IxpcQMsgType *rxIxpcMsg)
{
	int 	len;
    char    resBuf[4096];
    char    mmlBuf[4096];

	printAppConfig(resBuf);

	len = 0;
    len += sprintf(&mmlBuf[len], "\n  RESULT = SUCCESS\n  SYSTEM = %s\n\n", mySysName);
    len += sprintf(&mmlBuf[len], "%s\n\n", resBuf);
	mmclib_sendMmcResult(rxIxpcMsg, mmlBuf, RES_SUCCESS, FLAG_COMPLETE, 0, 0, 0);

	return 0;
}

int printFailureHeader(char *buff, const char *reason)
{
   int     len = 0;

	len += sprintf(&buff[len], "\n  SYSTEM     = %s\n", mySysName);
	len += sprintf(&buff[len], "  PROC       = %s\n", myAppName);
	len += sprintf(&buff[len], "  RESULT     = FAIL\n\n");
	len += sprintf(&buff[len], "  FAIL REASON = %s\n", reason);

	return len;
}

int printSuccessHeader(char *buff)
{
	int     len = 0;

	len += sprintf(&buff[len], "\n  SYSTEM    = %s\n", mySysName);
	len += sprintf(&buff[len], "  PROC      = %s\n", myAppName);
	len += sprintf(&buff[len], "  RESULT    = SUCCESS\n\n");

	return len;
}

int mmib_relay_mmc (IxpcQMsgType *rxIxpcMsg)
{
    GeneralQMsgType genMsg;
    MMLReqMsgType   *rxReqMsg;
    int i, j, txLen, len = 0;
    char buf[4096];
    char blkName[32];
    int destQid;

    rxReqMsg = (MMLReqMsgType *)rxIxpcMsg->body;

    memset(blkName, 0, sizeof(blkName));
    for (i=0; i<rxReqMsg->head.paraCnt; i++) {
        if (strcasecmp(rxReqMsg->head.para[i].paraName, "BLKNAME") == 0) {
            for(j = 0; j < (int)strlen(rxReqMsg->head.para[i].paraVal); j++)
                blkName[j] = toupper(rxReqMsg->head.para[i].paraVal[j]);
        }
    }

    if(strcmp(blkName, myAppName) == 0) {
        if(strcasecmp(rxReqMsg->head.cmdName, "reload-config-data") == 0)
            return reload_config_data(rxIxpcMsg);
        else if(strcasecmp(rxReqMsg->head.cmdName, "dis-config-data") == 0)
            return dis_config_data(rxIxpcMsg);
    }

	if (strcasecmp(blkName, "ixpc") == 0 || strcasecmp(blkName, "shmd") == 0) {
        len += sprintf(&buf[len], "\n  RESULT = FAIL\n  SYSTEM = %s\n\n", mySysName);
        len += sprintf(&buf[len], "  FAIL REASON = DO NOT USE THIS COMMAND FOR THIS BLOCK(%s)\n", blkName);
        mmclib_sendMmcResult(rxIxpcMsg, buf, RES_FAIL, FLAG_COMPLETE, 0, 0, 0);
        return -1;
    }

    ERRLOG(LL3, FL, "[%s] command relay to [%s] block.\n", rxReqMsg->head.cmdName, blkName);

    if ((destQid = commlib_crteMsgQ(l_sysconf, blkName, FALSE)) < 0) {
        len += sprintf(&buf[len], "\n  RESULT = FAIL\n  SYSTEM = %s\n\n", mySysName);
        len += sprintf(&buf[len], "  FAIL REASON =  GET QID ERROR(BLKNAME=%s)\n", blkName);
        mmclib_sendMmcResult(rxIxpcMsg, buf, RES_FAIL, FLAG_COMPLETE, 0, 0, 0);
        return -1;
    }

    genMsg.mtype = MTYPE_MMC_REQUEST;
    memcpy(genMsg.body, (char*)rxIxpcMsg, sizeof(IxpcQMsgType));

	txLen = sizeof(rxIxpcMsg->head) + rxIxpcMsg->head.bodyLen;
    if (msgsnd(destQid, (void*)&genMsg, txLen, IPC_NOWAIT) < 0) {
        lprintf(ELI, LLE, FL, "DIS_CONFIG_DATA msgsnd fail to %s. err=%d(%s)\n",
                        blkName, errno, strerror(errno));
        len += sprintf(&buf[len], "\n  RESULT = FAIL\n  SYSTEM = %s\n\n", mySysName);
        len += sprintf(&buf[len], "  FAIL REASON =  MSG SEND FAIL TO %s\n", blkName);
        mmclib_sendMmcResult(rxIxpcMsg, buf, RES_FAIL, FLAG_COMPLETE, 0, 0, 0);
        return -1;
    }

    return 0;
}

int dis_config_data(IxpcQMsgType *rxIxpcMsg)
{
	GeneralQMsgType 	genMsg;
	MMLReqMsgType   	*rxReqMsg;
	int i, j, txLen, len = 0;
	char 				mmlBuf[4096];
	char blkName[32];
	int destQid;

	memset(mmlBuf, 0x00, sizeof(mmlBuf) );

	rxReqMsg = (MMLReqMsgType *)rxIxpcMsg->body;

	memset(blkName, 0, sizeof(blkName));
	for (i=0; i<rxReqMsg->head.paraCnt; i++) {
		if (strcasecmp(rxReqMsg->head.para[i].paraName, "BLKNAME") == 0) {
			for(j = 0; j < (int)strlen(rxReqMsg->head.para[i].paraVal); j++)
				blkName[j] = toupper(rxReqMsg->head.para[i].paraVal[j]);
		}
	}

	ERRLOG(LL3, FL, "RECEIVED: dis-config-data blkName=[%s]\n", blkName);

	if ((destQid = commlib_crteMsgQ(l_sysconf, blkName, FALSE))<0 ) {
		len += sprintf(&mmlBuf[len], "\n  RESULT = FAIL\n  SYSTEM = %s\n\n", mySysName);
		len += sprintf(&mmlBuf[len], "  FAIL REASON =  GET QID ERROR(BLKNAME=%s)\n", blkName);
		mmclib_sendMmcResult(rxIxpcMsg, mmlBuf, RES_FAIL, FLAG_COMPLETE, 0, 0, 0);
		return -1;
	}

	genMsg.mtype = MTYPE_DIS_CONFIG_DATA;
	memcpy(genMsg.body, (char*)rxIxpcMsg, sizeof(IxpcQMsgType));

	txLen = sizeof(rxIxpcMsg->head) + rxIxpcMsg->head.bodyLen;
	if (msgsnd(destQid, (void*)&genMsg, txLen, IPC_NOWAIT) < 0) {
		lprintf(ELI, LLE, FL, "DIS_CONFIG_DATA msgsnd fail to %s. err=%d(%s)\n",
				blkName, errno, strerror(errno));
		len += sprintf(&mmlBuf[len], "\n  RESULT = FAIL\n  SYSTEM = %s\n\n", mySysName);
		len += sprintf(&mmlBuf[len], "  FAIL REASON =  MSG SEND FAIL TO %s\n", blkName);
		mmclib_sendMmcResult(rxIxpcMsg, mmlBuf, RES_FAIL, FLAG_COMPLETE, 0, 0, 0);
		return -1;
	}

	return 0;
}

int reload_config_data(IxpcQMsgType *rxIxpcMsg)
{
	GeneralQMsgType genMsg;
	MMLReqMsgType   *rxReqMsg;
	int i, j, txLen, len = 0;
	char mmlBuf[4096];
	char blkName[32];
	int destQid;

	memset(mmlBuf,     0x00, sizeof(mmlBuf) );

	rxReqMsg = (MMLReqMsgType *)rxIxpcMsg->body;

	memset(blkName, 0, sizeof(blkName));
	for (i=0; i<rxReqMsg->head.paraCnt; i++) {
		if (strcasecmp(rxReqMsg->head.para[i].paraName, "BLKNAME") == 0) {
			for(j = 0; j < (int)strlen(rxReqMsg->head.para[i].paraVal); j++)
				blkName[j] = toupper(rxReqMsg->head.para[i].paraVal[j]);
		}
	}

	ERRLOG(LL3, FL, "RECEIVED: reload-config-data blkName=[%s]\n", blkName);

	if ((destQid = commlib_crteMsgQ(l_sysconf, blkName, FALSE))<0 ) {
		len += sprintf(&mmlBuf[len], "\n  RESULT = FAIL\n  SYSTEM = %s\n\n", mySysName);
		len += sprintf(&mmlBuf[len], "  FAIL REASON =  GET QID ERROR(BLKNAME=%s)\n", blkName);
		mmclib_sendMmcResult(rxIxpcMsg, mmlBuf, RES_SUCCESS, FLAG_COMPLETE, 0, 0, 0);
		return -1;
	}

	genMsg.mtype = MTYPE_RELOAD_CONFIG_DATA;
	memcpy(genMsg.body, (char*)rxIxpcMsg, sizeof(IxpcQMsgType));

	txLen = sizeof(rxIxpcMsg->head) + rxIxpcMsg->head.bodyLen;
	if (msgsnd(destQid, (void*)&genMsg, txLen, IPC_NOWAIT) < 0) {
		lprintf(ELI, LLE, FL, "RELOAD_CONFIG_DATA msgsnd fail to %s. err=%d(%s)\n",
				blkName, errno, strerror(errno));
		len += sprintf(&mmlBuf[len], "\n  RESULT = FAIL\n  SYSTEM = %s\n\n", mySysName);
		len += sprintf(&mmlBuf[len], "  FAIL REASON =  MSG SEND FAIL TO %s\n", blkName);
		mmclib_sendMmcResult(rxIxpcMsg, mmlBuf, RES_FAIL, FLAG_COMPLETE, 0, 0, 0);
		return -1;
	}

	return 0;
}

