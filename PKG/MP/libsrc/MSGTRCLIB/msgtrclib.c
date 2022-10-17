
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <time.h>
#include <commlib.h>
#include <msgtrclib.h>

int			ixpcQid;

TraceList	*trcList;

extern char mySysName[COMM_MAX_NAME_LEN];
extern char myAppName[COMM_MAX_NAME_LEN];

int msgtrclib_init(void)
{
	int		err;

	if ((ixpcQid = commlib_crteMsgQ(l_sysconf, "IXPC", FALSE))<0 ) {
		fprintf(stderr, "[%s.%d] [%s] error, crteMsgQ(IXPC) failed\n", FL, __func__);
		return -1;
	}

	if ((err= commlib_crteShm(l_sysconf, "SHM_TRC_INFO", sizeof(TraceList), (void*)&trcList)) <0 ) {
		fprintf(stderr, "[%s.%d] [%s] Fail to get SHM_TRC_INFO \n", FL, __func__);
		return -1;
	}

	if (err == SHM_CREATE) {
		memset(trcList, 0x00, sizeof(TraceList));
	}

	return 0;
}

int msgtrclib_addTraceList(char *trcNum, time_t expiredTime, char *errStr, int test, int level)
{
	int		i;

    if(trcNum == NULL) {
		sprintf(errStr, "TRACE NUMBER IS NULL");
        return -1;
    }

	for (i=0; i<MAX_TRACE_LIST; i++) {
		// 존재 여부 체크 
		if (!strcmp(trcList->info[i].trcNum, trcNum)) {
			sprintf(errStr, "ALREADY REGISTERED TRACE NUMBER [%s]", trcNum);
			return -1; 
		}
	}

	if (trcList->listCnt == MAX_TRACE_LIST) {
		sprintf(errStr, "TRACE LIST IS FULL");
		return -1;
	}

	for (i = 0; i < MAX_TRACE_LIST; i++) {
		if (trcList->info[i].trcNum[0] == 0) {
			break;
		}
	}

	if(i == MAX_TRACE_LIST) {
		sprintf(errStr, "TRACE LIST IS FULL");
	   	return -1;
	}

	strcpy(trcList->info[i].trcNum, trcNum);
	trcList->info[i].trcSub[0][0] = 0;
	trcList->info[i].trcSub[1][0] = 0;
	if(expiredTime == 0 ) {
		trcList->info[i].expiredTime = 0;
		trcList->info[i].trcFlag =1;
	} else {
		trcList->info[i].expiredTime = expiredTime;
	}
	trcList->info[i].testRule = test;
	trcList->info[i].trc_level = level + 10 * test;

	trcList->listCnt++;
	return i;
}

int msgtrclib_delTraceList(char *trcNum, char *errStr)
{
	int		i;

    if(trcNum == NULL) {
		sprintf(errStr, "TRACE NUMBER IS NULL");
        return -1;
    }

	if (trcList->listCnt == 0) {
		sprintf(errStr, "TRACE LIST IS EMPTY ");
		//return -1;
	}

	if (strlen(trcNum) == 0 ) {
		sprintf(errStr, "INVALID TRACE NUMBER");
		return -1;
	}

	for(i=0; i<MAX_TRACE_LIST; i++) {
		if (strcasecmp(trcList->info[i].trcNum, trcNum) == 0) {
			break;
		}
	}

	if( i== MAX_TRACE_LIST ) {
		sprintf(errStr, "NOT REGISTERED TRACE NUMBER");
		return -1;
	}

	ERRLOG(LLE, FL, " CANCEL (%s) TRACE CANCELED \n",trcList->info[i].trcNum);

	memset(&trcList->info[i], 0x00 , sizeof(TraceInfo) );
	if( trcList->listCnt > 0)
	trcList->listCnt--;

	return 0;
}

void msgtrclib_sendTrcSyncMsg(void)
{
	GeneralQMsgType     txGenQMsg;
	IxpcQMsgType        *txIxpcMsg;
	int                 txLen, bodyLen=0;
	int                 len=0;
	time_t              now=0;

	memset (&txGenQMsg, 0x00, sizeof(GeneralQMsgType));

	txIxpcMsg = (IxpcQMsgType*) txGenQMsg.body;

	txGenQMsg.mtype = MTYPE_TRC_SYNC_NOTI;

	sprintf(txIxpcMsg->head.srcSysName, mySysName);
	strcpy (txIxpcMsg->head.srcAppName, myAppName);
	strcpy (txIxpcMsg->head.dstSysName, mySysName);
	strcpy (txIxpcMsg->head.dstAppName, "SRDC");

	txIxpcMsg->head.msgId   = 0;
	txIxpcMsg->head.segFlag = 0;
	txIxpcMsg->head.seqNo   = 0;
	txIxpcMsg->head.byteOrderFlag = BYTE_ORDER_TAG;

	txIxpcMsg->head.bodyLen = 0;

	txLen = sizeof(txIxpcMsg->head) + txIxpcMsg->head.bodyLen;

	if (msgsnd(ixpcQid, (char *)&txGenQMsg, txLen, IPC_NOWAIT) < 0){
		ERRLOG(LLE, FL,"[%s] msgsnd fail error(%d:%s)\n" ,__func__, errno, strerror(errno));
		return;
	}

	return;
}


int msgtrclib_traceOk(int type, int testRule, char *trcKey, char *subKey)
{
	char	tmpStr[32];
	int		i, cnt;

	if (type == TRC_CHECK_START) {
		if(strlen(trcKey) <= 0) return 0;
	} else {
		if(strlen(subKey) <= 0) return 0;
	}

	if (trcList->listCnt == 0) return 0;

	cnt = 0;
	for (i=0; i<MAX_TRACE_LIST; i++) {
		if (strlen(trcList->info[i].trcNum) <= 0)
			continue;

#if 0	// NOT USED
		if (type == TRC_CHECK_START) {
			if (!strcmp(trcList->info[i].trcNum, trcKey)) {
				if (strcmp(trcList->info[i].trcSub[pIdx] , subKey) != 0) {
					strcpy(trcList->info[i].trcSub[pIdx], subKey);
					msgtrclib_sendTrcSyncMsg();
				}

				if (trcList->info[i].testRule == 1) 
					return (trcList->info[i].trc_level + 10);
				return trcList->info[i].trc_level;
			}
		} else {
			if (!strcmp(trcList->info[i].trcSub[pIdx], subKey)) {
				sprintf(trcKey, "%s", trcList->info[i].trcNum);

				if (trcList->info[i].testRule == 1) 
					return (trcList->info[i].trc_level + 10);
				return trcList->info[i].trc_level;
			}
		}
#endif

		//if (!strcmp(trcList->info[i].trcNum, trcKey) && trcList->info[i].testRule == testRule) {
		if ((trcList->info[i].testRule == testRule) && !strcmp(trcList->info[i].trcNum, trcKey)) {
			return trcList->info[i].trc_level;
		}

		if(++cnt >= trcList->listCnt)
			return 0;
	}
	return 0;
}

void msgtrclib_makeTraceMsg(char *msg, char *key, char* addHead, char *data)
{
	int len;
	time_t now;
	char trcTime[32];
	struct timeval curr;

	gettimeofday(&curr, NULL);
	len = strftime(trcTime, 32, "%Y-%m-%d %H:%M:%S", localtime(&curr.tv_sec));
	len += sprintf(&trcTime[len], ".%03d", (int)(curr.tv_usec/1000));

	len = sprintf(msg, "===============================================================\n");
	len += sprintf(&msg[len], " TRACE TIME         : %s\n", trcTime);
	len += sprintf(&msg[len], " TRACE SYSTEM       : %s\n", mySysName);
	len += sprintf(&msg[len], " TRACE KEY          : %s\n", key);
	if(addHead)
		len += sprintf(&msg[len], "%s", addHead);
	len += sprintf(&msg[len], "---------------------------------------------------------------\n");
	len += sprintf(&msg[len], "%s", data);
}

void msgtrclib_sendTraceMsg(TraceMsgHead *head, char *msg)
{       
	char				*text;
	GeneralQMsgType     txGenQMsg;
	IxpcQMsgType        *txIxpcMsg;
	TraceMsgInfo        *txTrcMsg;
	int                 txLen, bodyLen=0;
	int                 len=0;
	time_t 				now=0;
	char 				trcTime[32];
	struct timeval 		curr;

	memset (&txGenQMsg, 0x00, sizeof(GeneralQMsgType));


	txIxpcMsg = (IxpcQMsgType*) txGenQMsg.body;

	txTrcMsg= (TraceMsgInfo*)txIxpcMsg->body;
	bodyLen = strlen(msg);

	memcpy(txTrcMsg->trcMsg, msg, bodyLen);

	if(msg == NULL){
		ERRLOG(LLE, FL,"[%s]Trace Msg is NULL \n",__func__);
		return;
	}

	if (strlen(head->primaryKey) > 0) {
		if (strncmp(head->primaryKey, "82", strlen("82")) == 0)
			sprintf(txTrcMsg->primaryKey, "0%s", &head->primaryKey[2]);
		else
			strcpy(txTrcMsg->primaryKey, head->primaryKey);
	} else {
		strcpy(txTrcMsg->primaryKey," ");
	}

	if (strlen(head->secondaryKey) > 0) {
		sprintf(txTrcMsg->secondaryKey, "%s", head->secondaryKey);
	} else {
		strcpy(txTrcMsg->secondaryKey," ");
	}

	strcpy(txTrcMsg->opCode, head->opCode);
	txTrcMsg->direction = head->direction;
	txTrcMsg->originSysType = head->originSysType;
	txTrcMsg->destSysType = head->destSysType;

	gettimeofday(&curr, NULL);
	len = strftime(trcTime, 32, "%Y-%m-%d %H:%M:%S", localtime(&curr.tv_sec));
	len += sprintf(&trcTime[len], ".%03d", (int)(curr.tv_usec/1000));
	strcpy(txTrcMsg->trcTime,trcTime);


	if (txTrcMsg->direction == 0) 
		txGenQMsg.mtype = MTYPE_TRC_CONSOLE;
	else
		txGenQMsg.mtype = MTYPE_TRC_DB_INSERT;

	sprintf (txIxpcMsg->head.srcSysName, "%s", mySysName);
	strcpy (txIxpcMsg->head.srcAppName, myAppName);
	strcpy (txIxpcMsg->head.dstSysName, "OMP");
	strcpy (txIxpcMsg->head.dstAppName, "COND");

	txIxpcMsg->head.msgId = MSGID_SYS_COMM_STATUS_REPORT;
	txIxpcMsg->head.segFlag = 0;
	txIxpcMsg->head.seqNo   = 0;
	txIxpcMsg->head.byteOrderFlag = BYTE_ORDER_TAG;

	txIxpcMsg->head.bodyLen = sizeof(TraceMsgInfo);

	txLen = sizeof(txGenQMsg.mtype) + sizeof(txIxpcMsg->head) + txIxpcMsg->head.bodyLen;

	if (msgsnd(ixpcQid, (char *)&txGenQMsg, txLen, IPC_NOWAIT) < 0){
		ERRLOG(LLE, FL,"[%s] msgsnd fail error(%d:%s)\n" ,__func__, errno, strerror(errno));
		return;
	}

	return;
}


void msgtrclib_checkTrcTime(time_t now)
{
	int 	i, retVal;
	char	errStr[1024];
	char	tmpStr[64];

	memset(errStr, 0x00, sizeof(errStr));

	for (i = 0; i < MAX_TRACE_LIST; i++) {

		if (strlen(trcList->info[i].trcNum) <= 0)
			continue;

		if (trcList->info[i].trcFlag == 1) // UNLIMITE 상태 
			continue;

		if (trcList->info[i].expiredTime - now <= 0) {
			memset(tmpStr, 0x00, sizeof(tmpStr));
			sprintf(tmpStr,"%s",trcList->info[i].trcNum);
			retVal = msgtrclib_delTraceList(trcList->info[i].trcNum, errStr);

			if (retVal < 0)
				ERRLOG(LLE, FL, "TRC DELETE FAIL[TRCNUM=%s REASON=%s]\n", tmpStr, errStr);
			else
				ERRLOG(LLE, FL, "TRC DELETE SUCCESS[TRCNUM=%s]\n", tmpStr);
		}
	}

	return;
}

void msgtrclib_sendAlarmMsg(int msgId_almCode, int almLevel, char *almDesc, char *almInfo)
{
    int                 txLen;

    GeneralQMsgType     txGenQMsg;
    IxpcQMsgType        *txIxpcMsg;
    AlmMsgInfo          almMsg;

    txIxpcMsg = (IxpcQMsgType*)txGenQMsg.body;
    memset ((void*)&txIxpcMsg->head, 0, sizeof(txIxpcMsg->head));
    memset (&almMsg, 0x00, sizeof(AlmMsgInfo));

    txGenQMsg.mtype = MTYPE_ALARM_REPORT;

    strcpy (txIxpcMsg->head.srcSysName, mySysName);
    strcpy (txIxpcMsg->head.srcAppName, myAppName);
    strcpy (txIxpcMsg->head.dstSysName, "OMP");
    strcpy (txIxpcMsg->head.dstAppName, "FIMD");

    txIxpcMsg->head.msgId   = msgId_almCode;
    txIxpcMsg->head.bodyLen = sizeof(AlmMsgInfo);

    txLen = sizeof(txIxpcMsg->head) + txIxpcMsg->head.bodyLen;

    almMsg.almCode = msgId_almCode;
    almMsg.almLevel = almLevel;
    sprintf(almMsg.almDesc, "%s", almDesc);
    sprintf(almMsg.almInfo, "%s", almInfo);
	
    memcpy ((void*)txIxpcMsg->body, &almMsg, sizeof(AlmMsgInfo));

    if (msgsnd(ixpcQid, (void*)&txGenQMsg, txLen, IPC_NOWAIT) < 0) {
		ERRLOG(LLE, FL,"[%s] send Alarm-Noti Message Fail (%d:%s)\n" ,__func__, errno, strerror(errno));
    }
}
