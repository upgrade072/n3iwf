/*
 * FILE: mmclib.c
 */

#include "mmclib.h"

static int numMmcHdlr;
static MmcHdlrVector* mmcHdlrVector;

static int mmcHdlrVector_qsortCmp (const void *a, const void *b)
{
	return (strcasecmp (((MmcHdlrVector*)a)->cmdName, ((MmcHdlrVector*)b)->cmdName));
}

static int mmcHdlrVector_bsrchCmp (const void *a, const void *b)
{
	return (strcasecmp ((char*)a, ((MmcHdlrVector*)b)->cmdName));
}


int mmclib_initMmc(MmcHdlrVector* pMmcHdlr)
{
	int i;

	mmcHdlrVector = pMmcHdlr;

	for(i = 0; i < MAX_MMC_HDLR; i++) {
		if(mmcHdlrVector[i].func == NULL)
			break;
	}
	numMmcHdlr = i;

	qsort ( (void*)mmcHdlrVector,
			numMmcHdlr,
			sizeof(MmcHdlrVector),
			mmcHdlrVector_qsortCmp );

	return 0;
}

void mmclib_handleMmcReq (IxpcQMsgType *rxIxpcMsg)
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
		ERRLOG(LL1, FL, "[%s] Unknown mml command. CMD=[%s]\n",
				__func__, mmlReqMsg->head.cmdName);
		return ;
	}

	/* 처리 function을 호출한다. */
	(int)(*(mmcHdlr->func)) (rxIxpcMsg);
}

int mmclib_sendMmcResult (IxpcQMsgType *rxIxpcMsg, char *buff, char resCode,
		char contFlag, unsigned short extendTime, char segFlag, char seqNo)
{
	int     txLen;
	GeneralQMsgType txGenQMsg;
	IxpcQMsgType    *txIxpcMsg;
	MMLResMsgType   *txMmlResMsg;
	MMLReqMsgType   *rxMmlReqMsg;

	txGenQMsg.mtype = MTYPE_MMC_RESPONSE;

	txIxpcMsg = (IxpcQMsgType*)txGenQMsg.body;
	memset ((void*)&txIxpcMsg->head, 0, sizeof(txIxpcMsg->head));

	txMmlResMsg = (MMLResMsgType*)txIxpcMsg->body;
	rxMmlReqMsg = (MMLReqMsgType*)rxIxpcMsg->body;

	// ixpc routing header
	strcpy (txIxpcMsg->head.srcSysName, mySysName);
	strcpy (txIxpcMsg->head.srcAppName, myAppName);
	strcpy (txIxpcMsg->head.dstSysName, rxIxpcMsg->head.srcSysName);
	strcpy (txIxpcMsg->head.dstAppName, rxIxpcMsg->head.srcAppName);
	txIxpcMsg->head.segFlag = segFlag;
	txIxpcMsg->head.seqNo   = seqNo;

	// mml result header
	txMmlResMsg->head.mmcdJobNo  = rxMmlReqMsg->head.mmcdJobNo;
	txMmlResMsg->head.extendTime = extendTime;
	txMmlResMsg->head.resCode    = resCode;
	txMmlResMsg->head.contFlag   = contFlag;
	strcpy (txMmlResMsg->head.cmdName, rxMmlReqMsg->head.cmdName);

	// result message
	strcat(buff, "\n");
	strcpy (txMmlResMsg->body, buff);

	txIxpcMsg->head.bodyLen = sizeof(txMmlResMsg->head) + strlen(buff) + 1;
	txLen = sizeof(txIxpcMsg->head) + txIxpcMsg->head.bodyLen;

	if (msgsnd(ixpcQid, (void*)&txGenQMsg, txLen, IPC_NOWAIT) < 0) {
		ERRLOG(LL1, FL, "[%s] Message send fail. DEST=IXPC ERR=%d(%s)\n",
				__func__, errno, strerror(errno));
		return -1;
	}

	return 0;
}


int mmclib_printFailureHeader(char *buff, char *reason)
{       
	int     len = 0;

	len += sprintf(&buff[len], "\n  SYSTEM     = %s\n", mySysName);
	len += sprintf(&buff[len], "  RESULT     = FAIL\n\n");
	len += sprintf(&buff[len], "  FAIL REASON = %s\n", reason);

	return len;
}

int mmclib_printSuccessHeader(char *buff)
{
	int     len = 0;

	len += sprintf(&buff[len], "\n  SYSTEM    = %s\n", mySysName);
	len += sprintf(&buff[len], "  RESULT    = SUCCESS\n\n");

	return len;
}

int mmclib_getMmlPara_IDX(IxpcQMsgType *rxIxpcMsg, char *paraName)
{
	int     i;
	MMLReqMsgType   *mmlReq = (MMLReqMsgType*)rxIxpcMsg->body;

	for (i=0; i < mmlReq->head.paraCnt; i++) {
		if (!strcasecmp (mmlReq->head.para[i].paraName, paraName)) {
			return i;
		}
	}
	return -1;
}

int mmclib_getMmlPara_STR(IxpcQMsgType *rxIxpcMsg, char *paraName, char *buff)
{
	int     i;
	MMLReqMsgType   *mmlReq = (MMLReqMsgType*)rxIxpcMsg->body;

	for (i=0; i < mmlReq->head.paraCnt; i++) {
		if (!strcasecmp (mmlReq->head.para[i].paraName, paraName)) {
			strcpy (buff, mmlReq->head.para[i].paraVal);
			return 1;
		}
	}
	buff[0] = 0;	// buff = NULL;
	return -1;
}

int mmclib_getMmlPara_INT(IxpcQMsgType *rxIxpcMsg, char *paraName)
{
	int     i;
	MMLReqMsgType   *mmlReq = (MMLReqMsgType*)rxIxpcMsg->body;

	for (i=0; i < mmlReq->head.paraCnt; i++) {
		if (!strcasecmp (mmlReq->head.para[i].paraName, paraName)) {
			return (int)strtol(mmlReq->head.para[i].paraVal, 0, 0);
		}
	}
	return -1;
}

int mmclib_getMmlPara_HEXA(IxpcQMsgType *rxIxpcMsg, char *paraName)
{
	int     i;
	MMLReqMsgType   *mmlReq = (MMLReqMsgType*)rxIxpcMsg->body;

	for (i=0; i < mmlReq->head.paraCnt; i++) {
		if (!strcasecmp (mmlReq->head.para[i].paraName, paraName)) {
			return (int)strtol (mmlReq->head.para[i].paraVal, 0, 16);
		}
	}
	return -1;
}


