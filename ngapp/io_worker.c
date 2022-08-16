#include "ngapp.h"
#define DEBUG_ASN	1

extern main_ctx_t *MAIN_CTX;
static __thread worker_ctx_t *WORKER_CTX;

void handle_ngap_send(int conn_fd, short events, void *data)
{
	recv_buf_t *buffer = (recv_buf_t *)data;
	ngap_msg_t *ngap_msg = (ngap_msg_t *)buffer->buffer;

	/* need free */
	json_object *js_ngap_pdu = NULL;
	NGAP_PDU_t *ngap_pdu = NULL;
	size_t xmlb_size = 0;
	xmlBuffer *xmlb = NULL;
	char *aper_string = NULL;
	size_t aper_size = 0;

	if ((js_ngap_pdu = json_tokener_parse(ngap_msg->msg_body)) == NULL) {
		goto HNS_END;
	}
	if ((xmlb = convert_json_to_xml(js_ngap_pdu, "NGAP-PDU", &xmlb_size, 0)) == NULL) {
		goto HNS_END;
	}
	if ((ngap_pdu = decode_pdu_to_ngap_asn(ATS_CANONICAL_XER, (char *)xmlb->content, xmlb_size, 0)) == NULL) {
		goto HNS_END;
	}
	if (check_asn_constraints(asn_DEF_NGAP_PDU, ngap_pdu, DEBUG_ASN) < 0) {
		goto HNS_END;
	}
	if ((aper_string = encode_asn_to_pdu_buffer(ATS_ALIGNED_BASIC_PER, asn_DEF_NGAP_PDU, ngap_pdu, &aper_size, 0)) == NULL) {
		goto HNS_END;
	}

	sctp_msg_t send_msg = { .mtype = 1 };
	memcpy(&send_msg.tag, &ngap_msg->sctp_tag, sizeof(sctp_tag_t));
	memcpy(&send_msg.msg_body, aper_string, aper_size);
	send_msg.msg_size = aper_size;
	int res = msgsnd(MAIN_CTX->QID_INFO.ngapp_sctpc_qid, &send_msg, SCTP_MSG_SIZE(&send_msg), IPC_NOWAIT);
	fprintf(stderr, "{dbg} send_relay res=[%s] size=(%ld) (%d:%s)\n", res == 0 ? "success" : "fail", SCTP_MSG_SIZE(&send_msg), errno, strerror(errno));
	
HNS_END:
	if (js_ngap_pdu) json_object_put(js_ngap_pdu);
	if (xmlb) xmlBufferFree(xmlb);
	if (ngap_pdu) ASN_STRUCT_FREE(asn_DEF_NGAP_PDU, ngap_pdu);
	if (aper_string) free(aper_string);

	buffer->occupied = 0;
}

void handle_ngap_recv(int conn_fd, short events, void *data)
{
	recv_buf_t *buffer = (recv_buf_t *)data;
	sctp_msg_t *sctp_msg = (sctp_msg_t *)buffer->buffer;

	NGAP_PDU_t *ngap_pdu = NULL;
	char *aper_string = NULL;
	size_t aper_size = 0;
	json_object *js_ngap_pdu = NULL;
	json_object *js_distr_key = NULL;

	if ((ngap_pdu = decode_pdu_to_ngap_asn(ATS_ALIGNED_BASIC_PER, sctp_msg->msg_body, sctp_msg->msg_size, 0)) == NULL) {
		goto HNR_END;
	}
	if (check_asn_constraints(asn_DEF_NGAP_PDU, ngap_pdu, DEBUG_ASN) < 0) {
		goto HNR_END;
	}
	if ((aper_string = encode_asn_to_pdu_buffer(ATS_CANONICAL_XER, asn_DEF_NGAP_PDU, ngap_pdu, &aper_size, 0)) == NULL) {
		goto HNR_END;
	}
	if ((js_ngap_pdu = convert_xml_to_jobj(aper_string, aper_size, asn_DEF_NGAP_PDU, 0)) == NULL) {
		goto HNR_END;
	}

	/* check distr & decide worker id */
	key_list_t key_list = {0,};
	js_distr_key = search_json_object_ex(js_ngap_pdu, MAIN_CTX->DISTR_INFO.worker_rule, &key_list); 
	int DISTR_ID = js_distr_key == NULL ?  -1 : json_object_get_int(js_distr_key);
	int QID = DISTR_ID < 0 ?  MAIN_CTX->QID_INFO.ngapp_nwucp_qid : MAIN_CTX->DISTR_INFO.worker_distr_qid[DISTR_ID % MAIN_CTX->DISTR_INFO.worker_num];
	fprintf(stderr, "{DBG} QID=(%d)\n", QID);

	ngap_msg_t recv_msg = { .mtype = 1 };
	memcpy(&recv_msg.sctp_tag, &sctp_msg->tag, sizeof(sctp_tag_t));
	recv_msg.ngap_tag.id = DISTR_ID;
	memcpy(&recv_msg.ngap_tag.key_list, &key_list, sizeof(key_list_t));
	recv_msg.msg_size = sprintf(recv_msg.msg_body, "%s", JS_PRINT_COMPACT(js_ngap_pdu));

	int res = msgsnd(QID, &recv_msg, NGAP_MSG_SIZE(&recv_msg), IPC_NOWAIT);
	fprintf(stderr, "{dbg} recv_relay res=[%s] size=(%ld) (%d:%s)\n", res == 0 ? "success" : "fail", NGAP_MSG_SIZE(&recv_msg), errno, strerror(errno));

HNR_END:
	if (ngap_pdu) ASN_STRUCT_FREE(asn_DEF_NGAP_PDU, ngap_pdu);
	if (aper_string) free(aper_string);
	if (js_ngap_pdu) json_object_put(js_ngap_pdu);
	if (js_distr_key) json_object_put(js_distr_key);

	buffer->occupied = 0;
}

void io_thrd_tick(evutil_socket_t fd, short events, void *data)
{
	//worker_ctx_t *worker_ctx = (worker_ctx_t *)data;
}

void *io_worker_thread(void *arg)
{
	WORKER_CTX = (worker_ctx_t *)arg;
	WORKER_CTX->evbase_thrd = event_base_new();

	struct timeval one_sec = {2, 0};
	struct event *ev_tick = event_new(WORKER_CTX->evbase_thrd, -1, EV_PERSIST, io_thrd_tick, WORKER_CTX);
	event_add(ev_tick, &one_sec);

	event_base_loop(WORKER_CTX->evbase_thrd, EVLOOP_NO_EXIT_ON_EMPTY);

	return NULL;
}

