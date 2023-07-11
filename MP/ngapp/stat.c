#include "ngapp.h"

extern __thread worker_ctx_t *WORKER_CTX;

const char ngap_stat_type_str[][128] = {
	"NGAP_RECV",
	"NGAP_SEND",
};

const char ngap_pdu_type_str[][128] = {
	"encodingFailMessage",
	"initiatingMessage",
	"successfulOutcome",
	"unsuccessfulOutcome",
	"ngapPduTypeUnknown",
};

int ngap_pdu_proc_code(NGAP_PDU *ngap_pdu)
{
	int proc_num = 0;

	switch (ngap_pdu->choice)
	{
		case initiatingMessage_chosen:
			proc_num = ngap_pdu->u.initiatingMessage.procedureCode;
			break;
		case successfulOutcome_chosen:
			proc_num = ngap_pdu->u.successfulOutcome.procedureCode;
			break;
		case unsuccessfulOutcome_chosen:
			proc_num =  ngap_pdu->u.unsuccessfulOutcome.procedureCode;
			break;
		default:
			proc_num = -1;
			break;
	}

	if (proc_num < 0 || proc_num >= NGAP_ProcCodeUnknown) {
		return -1;
	} else {
		return proc_num;
	}
}

void NGAP_STAT_COUNT(NGAP_PDU *ngap_pdu, ngap_stat_type_t stat_type)
{
	if (stat_type < 0 || stat_type >= NGAP_STAT_NUM) {
		return;
	}

	ngap_op_count_t *op_count = 
		&WORKER_CTX->ngap_stat.stat[WORKER_CTX->ngap_stat.ngap_stat_pos][stat_type];

	if (ngap_pdu == NULL) {
		/* en/decoding error count */
		op_count->count[NPTE_ngapPduTypeUnused][NGAP_ProcCodeUnknown]++;
	} else {
		int proc_num = ngap_pdu_proc_code(ngap_pdu);
		if (proc_num > 0) {
			/* correct proc num */
			op_count->count[ngap_pdu->choice][proc_num]++;
		} else {
			/* incorrect proc num */
			op_count->count[ngap_pdu->choice][NGAP_ProcCodeUnknown]++;
		}
	}
}

void NGAP_STAT_GATHER(main_ctx_t *MAIN_CTX, ngap_op_count_t STAT[NGAP_STAT_NUM])
{
	for (int i = 0; i < MAIN_CTX->IO_WORKERS.worker_num; i++) {
		ngap_stat_t *stat_worker = &MAIN_CTX->IO_WORKERS.workers[i].ngap_stat;
		int CUR_POS = stat_worker->ngap_stat_pos;
		stat_worker->ngap_stat_pos = (stat_worker->ngap_stat_pos + 1) % NGAP_STAT_SLOT;

		for (int stat = 0; stat < NGAP_STAT_NUM; stat++) {
			for (int type = 0; type < NPTE_ngapPduTypeUnknown; type++) {
				for (int proc = 0; proc < NGAP_ProcCodeUnknown; proc++) {
					STAT[stat].count[type][proc] += stat_worker->stat[CUR_POS][stat].count[type][proc];
					stat_worker->stat[CUR_POS][stat].count[type][proc] = 0;
				}
			}
		}
	}
}

void NGAP_STAT_PRINT(ngap_op_count_t STAT[NGAP_STAT_NUM])
{
	for (int stat = 0; stat < NGAP_STAT_NUM; stat++) {
		int stat_print = 0;
		for (int type = 0; type < NPTE_ngapPduTypeUnknown; type++) {
			int type_print = 0;
			for (int proc = 0; proc < NGAP_ProcCodeUnknown; proc++) {
				if (STAT[stat].count[type][proc] != 0) {
					if (stat_print == 0) {
						TRCLOG(LLE, FL, "=== %s ===\n", ngap_stat_type_str[stat]);
						stat_print = 1;
					}
					if (type_print == 0) {
						TRCLOG(LLE, FL,  "[%s]\n", ngap_pdu_type_str[type]);
						type_print = 1;
					}
					TRCLOG(LLE, FL, "  %-32s : %d\n", NGAP_PROC_C_STR(proc), STAT[stat].count[type][proc]);
				}
			}
		}
	}
}

void send_conn_stat(main_ctx_t *MAIN_CTX, IxpcQMsgType *rxIxpcMsg)
{
	int len = sizeof(int), txLen = 0;   
    int statCnt = 0;
            
    GeneralQMsgType sxGenQMsg;
    memset(&sxGenQMsg, 0x00, sizeof(GeneralQMsgType));
        
    IxpcQMsgType *sxIxpcMsg = (IxpcQMsgType*)sxGenQMsg.body;
    
    STM_CommonStatMsgType *commStatMsg=(STM_CommonStatMsgType *)sxIxpcMsg->body;
    STM_CommonStatMsg     *commStatItem=NULL;
    
    sxGenQMsg.mtype = MTYPE_STATISTICS_REPORT;
    sxIxpcMsg->head.msgId = MSGID_NGAP_STATISTIC_REPORT;
    sxIxpcMsg->head.seqNo = 0; // start from 1
            
    strcpy(sxIxpcMsg->head.srcSysName, rxIxpcMsg->head.dstSysName);
    strcpy(sxIxpcMsg->head.srcAppName, rxIxpcMsg->head.dstAppName);
    strcpy(sxIxpcMsg->head.dstSysName, rxIxpcMsg->head.srcSysName);
    strcpy(sxIxpcMsg->head.dstAppName, rxIxpcMsg->head.srcAppName);

	ngap_op_count_t STAT[NGAP_STAT_NUM];
	memset(&STAT, 0x00, sizeof(STAT));

	NGAP_STAT_GATHER(MAIN_CTX, STAT);
	NGAP_STAT_PRINT(STAT);

	for (int stat = 0; stat < NGAP_STAT_NUM; stat++) {
		for (int type = 0; type < NPTE_ngapPduTypeUnknown; type++) {

			commStatItem = &commStatMsg->info[statCnt]; ++statCnt;
			memset(commStatItem, 0x00, sizeof(STM_CommonStatMsg));
			len += sizeof (STM_CommonStatMsg);

			snprintf(commStatItem->strkey1, sizeof(commStatItem->strkey1), "%s", ngap_stat_type_str[stat]);
			snprintf(commStatItem->strkey2, sizeof(commStatItem->strkey2), "%s", ngap_pdu_type_str[type]);

			for (int proc = 0; proc < NGAP_ProcCodeUnknown; proc++) {
				commStatItem->ldata[proc] = STAT[stat].count[type][proc];
			}
		}
	}
	commStatMsg->num = statCnt;
	ERRLOG(LLE, FL, "[NGAP / STAT Write Succeed stat_count=(%d) sedNo=(%d) body/txLen=(%d:%d)]\n",
			statCnt, sxIxpcMsg->head.seqNo, len, txLen);

	sxIxpcMsg->head.segFlag = 0;
	sxIxpcMsg->head.seqNo++;
	sxIxpcMsg->head.bodyLen = len;
	txLen = sizeof(sxIxpcMsg->head) + sxIxpcMsg->head.bodyLen;

	if (msgsnd(MAIN_CTX->QID_INFO.ixpc_qid, (void*)&sxGenQMsg, txLen, IPC_NOWAIT) < 0) {
		ERRLOG(LLE, FL, "DBG] %s() status send fail IXPC qid[%d] err[%s]\n", __func__, MAIN_CTX->QID_INFO.ixpc_qid, strerror(errno));
	}
}
