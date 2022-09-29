#include "ngapp.h"
#define DEBUG_ASN	1

extern main_ctx_t *MAIN_CTX;
static __thread worker_ctx_t *WORKER_CTX;
static __thread struct ossGlobal w;
static __thread struct ossGlobal *world;
static __thread int PDU_NUM;

void handle_ngap_send(int conn_fd, short events, void *data)
{
	recv_buf_t *buffer = (recv_buf_t *)data;
	ngap_msg_t *ngap_msg = (ngap_msg_t *)buffer->buffer;

	/* decode pdu - (from JER) */
	ossSetEncodingRules(world, OSS_JSON);
	OssBuf json_buf = { .length = (long)ngap_msg->msg_size, .value = (unsigned char *)ngap_msg->msg_body };
	NGAP_PDU *ngap_pdu = NULL;
	if (ossDecode(world, &PDU_NUM, &json_buf, (void **)&ngap_pdu) != 0) {
		fprintf(stderr, "%s() fail to decode [json -> pdu] !\n", __func__);
		ossPrint(world, "%s\n", ossGetErrMsg(world));
		goto HNS_END;
	}

	/* print pdu */
	ossPrintPDU(world, PDU_NUM, ngap_pdu);

	/* encode pdu - (to APER) */
	ossSetEncodingRules(world, OSS_PER_ALIGNED);
	OssBuf pdu_buf = { .length = 0, .value = NULL };
	if (ossEncode(world, PDU_NUM, ngap_pdu, &pdu_buf) != 0) {
		fprintf(stderr, "%s() fail to encode [pdu -> ngap] !\n", __func__);
		ossPrint(world, "%s\n", ossGetErrMsg(world));
		goto HNS_END;
	}

	sctp_msg_t send_msg = { .mtype = 1 };
	memcpy(&send_msg.tag, &ngap_msg->sctp_tag, sizeof(sctp_tag_t));
	memcpy(&send_msg.msg_body, pdu_buf.value, pdu_buf.length);
	send_msg.msg_size = pdu_buf.length;
	int res = msgsnd(MAIN_CTX->QID_INFO.ngapp_sctpc_qid, &send_msg, SCTP_MSG_SIZE(&send_msg), IPC_NOWAIT);
	fprintf(stderr, "{dbg} send_relay res=[%s] size=(%ld) (%d:%s)\n", res == 0 ? "success" : "fail", SCTP_MSG_SIZE(&send_msg), errno, strerror(errno));

HNS_END:
	ossFreePDU(world, PDU_NUM, ngap_pdu);
	ossFreeBuf(world, pdu_buf.value);

	buffer->occupied = 0;
}

json_object *extract_distr_key(json_object *js_ngap_pdu, key_list_t *key_list)
{
	config_setting_t *cf_distr_rule = config_lookup(&MAIN_CTX->CFG, "distr_info.ngap_distr_rule");
	json_object	*js_distr_key = NULL;

	for (int i = 0; i < config_setting_length(cf_distr_rule); i++) {
		const char *distr_rule = config_setting_get_string_elem(cf_distr_rule, i);
		memset(key_list, 0x00, sizeof(key_list_t) - sizeof(key_list->key_val));
		js_distr_key = search_json_object_ex(js_ngap_pdu, distr_rule, key_list);
		if (js_distr_key != NULL) {
			return js_distr_key;
		}
	}

	return NULL;
}

void handle_ngap_recv(int conn_fd, short events, void *data)
{
	recv_buf_t *buffer = (recv_buf_t *)data;
	sctp_msg_t *sctp_msg = (sctp_msg_t *)buffer->buffer;

	/* decode pdu - (from APER) */
	ossSetEncodingRules(world, OSS_PER_ALIGNED);
	OssBuf ngap_buf = { .length = (long)sctp_msg->msg_size, .value = (unsigned char *)sctp_msg->msg_body };
	NGAP_PDU *ngap_pdu = NULL;
	if (ossDecode(world, &PDU_NUM, &ngap_buf, (void **)&ngap_pdu) != 0) {
		fprintf(stderr, "%s() fail to decode [ngap -> pdu] !\n", __func__);
		ossPrint(world, "%s\n", ossGetErrMsg(world));
		goto HNR_END;
	}

	/* print pdu */
	ossPrintPDU(world, PDU_NUM, ngap_pdu);

	/* encode pdu - (to JER) */
	ossSetEncodingRules(world, OSS_JSON);
	ossSetJsonFlags(world, ossGetJsonFlags(world) | JSON_ENC_CONTAINED_AS_TEXT | JSON_COMPACT_ENCODING);
	OssBuf json_buf = { .length = 0, .value = NULL };
	if (ossEncode(world, PDU_NUM, ngap_pdu, &json_buf) != 0) {
		fprintf(stderr, "%s() fail to encode [pdu -> json] !\n", __func__);
		ossPrint(world, "%s\n", ossGetErrMsg(world));
		goto HNR_END;
	}

	/* check distr rule */
	json_object	*js_ngap_pdu = json_tokener_parse((const char *)json_buf.value);
	key_list_t key_list = {0,};
	json_object	*js_distr_key = extract_distr_key(js_ngap_pdu, &key_list);

	int distr_id = js_distr_key == NULL ?  -1 : json_object_get_int(js_distr_key);
	int qid = distr_id < 0 ?  MAIN_CTX->QID_INFO.ngapp_nwucp_qid : MAIN_CTX->DISTR_INFO.worker_distr_qid;
	json_object_put(js_ngap_pdu);

	/* MSGQID = nwucp main or nwucp worker, MSGID = if main -> 1 or if worker -> worker_index (1+worker_id) */
	ngap_msg_t recv_msg = { .mtype = distr_id < 0 ? 1 : (distr_id % MAIN_CTX->DISTR_INFO.worker_num) + 1 };
	memcpy(&recv_msg.sctp_tag, &sctp_msg->tag, sizeof(sctp_tag_t));
	recv_msg.ngap_tag.id = distr_id;
	memcpy(&recv_msg.ngap_tag.key_list, &key_list, sizeof(key_list_t));
	recv_msg.msg_size = sprintf(recv_msg.msg_body, "%s", json_buf.value);

	int res = msgsnd(qid, &recv_msg, NGAP_MSG_SIZE(&recv_msg), IPC_NOWAIT);
	fprintf(stderr, "{dbg} recv_relay res=[%s] size=(%ld) (%d:%s)\n", res == 0 ? "success" : "fail", NGAP_MSG_SIZE(&recv_msg), errno, strerror(errno));

HNR_END:
	ossFreePDU(world, PDU_NUM, ngap_pdu);
	ossFreeBuf(world, json_buf.value);

	buffer->occupied = 0;
}

void io_thrd_tick(evutil_socket_t fd, short events, void *data)
{
	//worker_ctx_t *worker_ctx = (worker_ctx_t *)data;
}

void *io_worker_thread(void *arg)
{
	/* prepare asn1 context per thread */
	int res = ossDupWorld(&MAIN_CTX->world, &w);
	if (res != 0) {
		fprintf(stderr, "{dbg} fail to prepare asn1 ctx at io_worker! = (%d)\n", res);
		exit(0);
	} else {
		world = &w;
		PDU_NUM = MAIN_CTX->pdu_num;
	}

	WORKER_CTX = (worker_ctx_t *)arg;
	WORKER_CTX->evbase_thrd = event_base_new();

	struct timeval one_sec = {2, 0};
	struct event *ev_tick = event_new(WORKER_CTX->evbase_thrd, -1, EV_PERSIST, io_thrd_tick, WORKER_CTX);
	event_add(ev_tick, &one_sec);

	event_base_loop(WORKER_CTX->evbase_thrd, EVLOOP_NO_EXIT_ON_EMPTY);

	return NULL;
}

