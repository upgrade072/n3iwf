#include "ngapp.h"

extern __thread worker_ctx_t *WORKER_CTX;

const char ngap_stat_type_str[][128] = {
	"NGAP_STAT_RECV",
	"NGAP_STAT_SEND",
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

void NGAP_STAT_PRINT(main_ctx_t *MAIN_CTX)
{
	ngap_op_count_t STAT[NGAP_STAT_NUM];
	memset(&STAT, 0x00, sizeof(STAT));

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

	for (int stat = 0; stat < NGAP_STAT_NUM; stat++) {
		int stat_print = 0;
		for (int type = 0; type < NPTE_ngapPduTypeUnknown; type++) {
			int type_print = 0;
			for (int proc = 0; proc < NGAP_ProcCodeUnknown; proc++) {
				if (STAT[stat].count[type][proc] != 0) {
					if (stat_print == 0) {
						fprintf(stderr, "=== %s ===\n", ngap_stat_type_str[stat]);
						stat_print = 1;
					}
					if (type_print == 0) {
						fprintf(stderr, "[%s]\n", ngap_pdu_type_str[type]);
						type_print = 1;
					}
					fprintf(stderr, "  %s : %d\n", NGAP_PROC_C_STR(proc), STAT[stat].count[type][proc]);
				}
			}
		}
	}
}
