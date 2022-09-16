#include <nwucp.h>

extern main_ctx_t *MAIN_CTX;
__thread worker_ctx_t *WORKER_CTX;

worker_ctx_t *worker_ctx_get_by_index(int index)
{
	return &MAIN_CTX->IO_WORKERS.workers[index % MAIN_CTX->IO_WORKERS.worker_num];
}

uint8_t create_eap_id()
{
	srand((unsigned int)time(NULL));
	uint8_t id = rand();
	return id;
}

void eap_req_snd_msg(ue_ctx_t *ue_ctx, int msg_id, const char *nas_pdu)
{
	eap_relay_t *eap_5g = &ue_ctx->eap_5g;
	memset(eap_5g, 0x00, sizeof(eap_relay_t));

	/* make eap start message at ue_ctx */
	eap_5g->eap_code = EAP_REQUEST;
	eap_5g->eap_id = create_eap_id();
	eap_5g->msg_id = msg_id;
	if (nas_pdu != NULL) {
		sprintf(eap_5g->nas_str, "%s", nas_pdu);
	}

	/* make IKE_AUTH_RES */
	ike_msg_t msg = { .mtype = 1 }, *ike_msg = &msg;
	// ue_id part
	memcpy(&ike_msg->ike_tag, &ue_ctx->ike_tag, sizeof(ike_tag_t));
	// n3iwf_msg part
	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;
	n3iwf_msg->msg_code = N3_IKE_AUTH_RES;
	n3iwf_msg->res_code = N3_EAP_REQUEST;
	memcpy(&n3iwf_msg->ctx_info, &ue_ctx->ctx_info, sizeof(ctx_info_t));
	// eap_5g_part
	memcpy(&ike_msg->eap_5g, eap_5g, sizeof(eap_relay_t));

	/* start timer */
	ue_ctx_stop_timer(ue_ctx);
	struct timeval tmout_sec = { N3_EXPIRE_TM_SEC, 0 };
	ue_ctx->ev_timer = event_new(WORKER_CTX->evbase_thrd, -1, EV_TIMEOUT, ue_ctx_release, ue_ctx);
	event_add(ue_ctx->ev_timer, &tmout_sec);

	/* send message to EAP5G */
	int res = msgsnd(MAIN_CTX->QID_INFO.nwucp_eap5g_qid, ike_msg, IKE_MSG_SIZE, IPC_NOWAIT);
	fprintf(stderr, "{dbg} %s send res=(%d)\n", __func__, res);
}

void tcp_req_snd_msg(ue_ctx_t *ue_ctx, int msg_id, const char *nas_pdu)
{
}

void eap_req_5g_start(int conn_fd, short events, void *data)
{
	ue_ctx_t *ue_ctx = (ue_ctx_t *)data;

	eap_req_snd_msg(ue_ctx, NAS_5GS_START, NULL); 
	ue_ctx->state = "EAP_REQ_5G_START";
}

void eap_req_5g_nas(ue_ctx_t *ue_ctx, const char *nas_pdu)
{
	eap_req_snd_msg(ue_ctx, NAS_5GS_NAS, nas_pdu); 
	ue_ctx->state = "EAP_REQ_5G_NAS";
}

void tcp_req_5g_nas(ue_ctx_t *ue_ctx, const char *nas_pdu)
{
	tcp_req_snd_msg(ue_ctx, NAS_5GS_NAS, nas_pdu); 
	ue_ctx->state = "TCP_REQ_5G_NAS";
}

void ngap_proc_initial_ue_message(ue_ctx_t *ue_ctx, ike_msg_t *ike_msg)
{
	eap_relay_t *eap_recv = &ike_msg->eap_5g;

	if (eap_recv->eap_code != EAP_RESPONSE && eap_recv->msg_id != NAS_5GS_NAS) {
		fprintf(stderr, "{todo} %s() recv invalid eap_id or msg_id!\n", __func__);
		goto NPIUM_ERR;
	}

	if (strlen(ike_msg->ike_tag.amf_host) == 0) {
		fprintf(stderr, "{todo} %s() recv invalid amf_select!\n", __func__);
		goto NPIUM_ERR;
	} else {
		sprintf(ue_ctx->amf_tag.amf_host, "%s", ike_msg->ike_tag.amf_host);
		fprintf(stderr, "{dbg} %s() save amf_host=[%s] to ue_ctx\n", __func__, ue_ctx->amf_tag.amf_host);
	}

	if (strlen(eap_recv->nas_str) == 0) {
		fprintf(stderr, "{todo} %s() recv invalid nas_pdu!\n", __func__);
		goto NPIUM_ERR;
	} else {
		fprintf(stderr, "{dbg} %s() recv nas_pdu=[%s]\n", __func__, ike_msg->eap_5g.nas_str);
	}

	if (eap_recv->an_param.cause.set == 0) {
		fprintf(stderr, "{todo} %s() recv invalid cause!\n", __func__);
		goto NPIUM_ERR;
	} else {
		fprintf(stderr, "{dbg} %s() recv cause=[%s]\n", __func__, eap_recv->an_param.cause.cause_str);
	}

	json_object *js_initial_ue_message = create_initial_ue_message_json(ue_ctx->index, eap_recv->nas_str, eap_recv->an_param.cause.cause_str);
	fprintf(stderr, "{DBG}\n%s\n", JS_PRINT_PRETTY(js_initial_ue_message));

	/* start timer */
	ue_ctx_stop_timer(ue_ctx);
	struct timeval tmout_sec = { N3_EXPIRE_TM_SEC, 0 };
	ue_ctx->ev_timer = event_new(WORKER_CTX->evbase_thrd, -1, EV_TIMEOUT, ue_ctx_release, ue_ctx);
	event_add(ue_ctx->ev_timer, &tmout_sec);
	ue_ctx->state = "NGAP_REQ_REGI";

	/* send to amf */
	ngap_send_json(ue_ctx->amf_tag.amf_host, JS_PRINT_COMPACT(js_initial_ue_message));

	/* release resource */
	json_object_put(js_initial_ue_message);

	return;

NPIUM_ERR:
	return ue_ctx_unset(ue_ctx);
}

void ngap_send_uplink_nas(ue_ctx_t *ue_ctx, char *nas_str)
{
	json_object *js_uplink_nas_transport_message = 
		create_uplink_nas_transport_json(ue_ctx->amf_tag.amf_ue_id, ue_ctx->amf_tag.ran_ue_id, nas_str);
	fprintf(stderr, "{dbg}\n%s\n", JS_PRINT_PRETTY(js_uplink_nas_transport_message));

	/* start timer */
	ue_ctx_stop_timer(ue_ctx);
	struct timeval tmout_sec = { N3_EXPIRE_TM_SEC, 0 };
	ue_ctx->ev_timer = event_new(WORKER_CTX->evbase_thrd, -1, EV_TIMEOUT, ue_ctx_release, ue_ctx);
	event_add(ue_ctx->ev_timer, &tmout_sec);
	ue_ctx->state = "NGAP_NAS_TRNAS";

	/* send to amf */
	ngap_send_json(ue_ctx->amf_tag.amf_host, JS_PRINT_COMPACT(js_uplink_nas_transport_message));

	/* release resource */
	json_object_put(js_uplink_nas_transport_message);
}

void ngap_proc_uplink_nas_transport(ue_ctx_t *ue_ctx, ike_msg_t *ike_msg)
{
	eap_relay_t *eap_recv = &ike_msg->eap_5g;

	if (eap_recv->eap_code != EAP_RESPONSE && eap_recv->msg_id != NAS_5GS_NAS) {
		fprintf(stderr, "{todo} %s() recv invalid eap_id or msg_id!\n", __func__);
		goto NPUNT_ERR;
	}

	if (strlen(eap_recv->nas_str) == 0) {
		fprintf(stderr, "{todo} %s() recv invalid nas_pdu!\n", __func__);
		goto NPUNT_ERR;
	} else {
		fprintf(stderr, "{dbg} %s() recv nas_pdu=[%s]\n", __func__, ike_msg->eap_5g.nas_str);
	}

	return ngap_send_uplink_nas(ue_ctx, ike_msg->eap_5g.nas_str);

NPUNT_ERR:
	return ue_ctx_unset(ue_ctx);
}

void nas_relay_to_amf(ike_msg_t *ike_msg)
{
	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;
	ue_ctx_t *ue_ctx = ue_ctx_get_by_index(n3iwf_msg->ctx_info.cp_id, WORKER_CTX);

	if (ue_ctx == NULL) {
		fprintf(stderr, "{todo} %s() called null ue_ctx! reply to UP fail response\n", __func__);
		return;
	}
	ue_ctx_stop_timer(ue_ctx);

	/* check eap_id */
	eap_relay_t *eap_send = &ue_ctx->eap_5g;
	eap_relay_t *eap_recv = &ike_msg->eap_5g;

	if (eap_send->eap_id != eap_recv->eap_id) {
		fprintf(stderr, "{todo} %s() check eap_id mismatch send=(%d) recv=(%d)\n", __func__, eap_send->eap_id, eap_recv->eap_id);
		return ue_ctx_unset(ue_ctx);
	}

	if (!strcmp(ue_ctx->state, "EAP_REQ_5G_START")) {
		return ngap_proc_initial_ue_message(ue_ctx, ike_msg);
	} else if (!strcmp(ue_ctx->state, "EAP_REQ_5G_NAS")) {
		return ngap_proc_uplink_nas_transport(ue_ctx, ike_msg);
	}
}

void nas_relay_to_ue(ngap_msg_t *ngap_msg, json_object *js_ngap_pdu)
{
	ue_ctx_t *ue_ctx = ue_ctx_get_by_index(ngap_msg->ngap_tag.id, WORKER_CTX);
	if (ue_ctx == NULL) {
		fprintf(stderr, "{todo} %s() fail to get ue (index:%d)\n", __func__, ngap_msg->ngap_tag.id);
		goto NRTU_ERR;
	} else {
		fprintf(stderr, "{dbg} %s() success to get ue (index:%d)\n", __func__, ue_ctx->index);
	}

	/* stop timer */
	ue_ctx_stop_timer(ue_ctx);

	/* check ngap message, have mandatory */
	key_list_t key_amf_ue_ngap_id = {0,};
	json_object *js_amf_ue_ngap_id = 
		search_json_object_ex(js_ngap_pdu, "/initiatingMessage/value/protocolIEs/{id:10, value}", &key_amf_ue_ngap_id);

	key_list_t key_ran_ue_ngap_id = {0,};
	json_object *js_ran_ue_ngap_id = 
		search_json_object_ex(js_ngap_pdu, "/initiatingMessage/value/protocolIEs/{id:85, value}", &key_ran_ue_ngap_id);

	key_list_t key_nas_pdu = {0,};
	json_object *js_nas_pdu = 
		search_json_object_ex(js_ngap_pdu, "/initiatingMessage/value/protocolIEs/{id:38, value}/", &key_nas_pdu);

	if (js_amf_ue_ngap_id == NULL || js_ran_ue_ngap_id == NULL || js_nas_pdu == NULL) {
		fprintf(stderr, "{todo} %s() fail to get mandatory amf_id, ran_id, nas_pdu!\n", __func__);
		goto NRTU_ERR;
	} else {
		ue_ctx->amf_tag.amf_ue_id = atoi(JS_PRINT_PRETTY(js_amf_ue_ngap_id));
		ue_ctx->amf_tag.ran_ue_id = atoi(JS_PRINT_PRETTY(js_ran_ue_ngap_id));
		fprintf(stderr, "{dbg} %s() recv amf_ue_id=(%d) ran_ue_id=(%d)\n", 
				__func__, ue_ctx->amf_tag.amf_ue_id, ue_ctx->amf_tag.ran_ue_id);
	}

	if (strcmp(ue_ctx->state, "NAS_RES_REGI_ACCEPT")) {
		/* Signalling SA not yet created */
		return eap_req_5g_nas(ue_ctx, JS_PRINT_PRETTY(js_nas_pdu));
	} else {
#if 0
		/* Signalling SA already created */
		return tcp_req_5g_nas(ue_cx, JS_PRINT_PRETTY(js_nas_pdu));
#endif
	}

NRTU_ERR:
	if (ue_ctx != NULL) {
		ue_ctx_unset(ue_ctx);
	}
}

void handle_ngap_msg(ngap_msg_t *ngap_msg, event_caller_t caller)
{
	sctp_tag_t *sctp_tag = &ngap_msg->sctp_tag;
	fprintf(stderr, "{dbg} from sctp [h:%s a:%d s:%d p:%d]\n", 
			sctp_tag->hostname, sctp_tag->assoc_id, sctp_tag->stream_id, sctp_tag->ppid);

	/* caution! must put this object */
	json_object *js_ngap_pdu = json_tokener_parse(ngap_msg->msg_body);
	fprintf(stderr, "{dbg} %s() recv ngap_pdu\n%s\n", __func__, JS_PRINT_PRETTY(js_ngap_pdu));

	key_list_t *key_list = &ngap_msg->ngap_tag.key_list;
	memset(key_list, 0x00, sizeof(key_list_t));

	int proc_code = json_object_get_int(search_json_object_ex(js_ngap_pdu, "/*/procedureCode", key_list));
	int success = !strcmp(key_list->key_val[0], "successfulOutcome") ? true : false;

	switch (proc_code) 
	{
		case NGSetupResponse:
			amf_regi_res_handle(sctp_tag, success, js_ngap_pdu);
			break;
		case DownlinkNASTransport:
			nas_relay_to_ue(ngap_msg, js_ngap_pdu);
			break;
		default:
			/* we can't handle, just discard */
			break;
	}

	if (js_ngap_pdu != NULL) {
		json_object_put(js_ngap_pdu);
	}
}

void handle_ike_msg(ike_msg_t *ike_msg, event_caller_t caller)
{
	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;
	eap_relay_t *eap_5g = &ike_msg->eap_5g;

	switch (n3iwf_msg->msg_code)
	{
		case N3_IKE_AUTH_INIT:
			return ue_assign_by_ike_auth(ike_msg);
		case N3_IKE_AUTH_REQ:
			if (eap_5g->an_param.set == 1) {
				/* main (select amf) --msgsnd--> worker */
				return ue_set_amf_by_an_param(ike_msg);
			} else {
				return nas_relay_to_amf(ike_msg);
			}
			return;
		default:
			break;
	}
}

void msg_rcv_from_ngapp(int conn_fd, short events, void *data)
{
	const char *caller = (const char *)data;
	int ev_caller = !strcmp(caller, "main") ? EC_MAIN : EC_WORKER;

	ngap_msg_t msg = {0,}, *ngap_msg = &msg;

	while (msgrcv(
			ev_caller == EC_MAIN ? MAIN_CTX->QID_INFO.ngapp_nwucp_qid : MAIN_CTX->QID_INFO.ngapp_nwucp_worker_qid,
			ngap_msg, sizeof(ngap_msg_t), 
			ev_caller == EC_MAIN ? 0 : WORKER_CTX->thread_index + 1,
			IPC_NOWAIT) > 0) {

		fprintf(stderr, "{dbg} %s() [%s:%s] recv msg!\n", __func__, caller, strcmp(caller, "main") ? WORKER_CTX->thread_name : "");

		handle_ngap_msg(ngap_msg, ev_caller);
	}
}

void msg_rcv_from_eap5g(int conn_fd, short events, void *data)
{
	const char *caller = (const char *)data;
	int ev_caller = !strcmp(caller, "main") ? EC_MAIN : EC_WORKER;

	ike_msg_t msg = {0,}, *ike_msg = &msg;

	while (msgrcv(
			ev_caller == EC_MAIN ? MAIN_CTX->QID_INFO.eap5g_nwucp_qid : MAIN_CTX->QID_INFO.eap5g_nwucp_worker_qid,
			ike_msg, sizeof(ike_msg_t), 
			ev_caller == EC_MAIN ? 0 : WORKER_CTX->thread_index + 1,
			IPC_NOWAIT) > 0) {

		fprintf(stderr, "{dbg} %s() [%s:%s] recv msg!\n", __func__, caller, strcmp(caller, "main") ? WORKER_CTX->thread_name : "");

		handle_ike_msg(ike_msg, ev_caller);
	}
}

void io_thrd_tick(int conn_fd, short events, void *data)
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

    struct timeval u_sec = {0, 1};
    struct event *ev_msg_rcv_from_ngapp = event_new(WORKER_CTX->evbase_thrd, -1, EV_PERSIST, msg_rcv_from_ngapp, "worker");
    event_add(ev_msg_rcv_from_ngapp, &u_sec);
    struct event *ev_msg_rcv_from_eap5g = event_new(WORKER_CTX->evbase_thrd, -1, EV_PERSIST, msg_rcv_from_eap5g, "worker");
    event_add(ev_msg_rcv_from_eap5g, &u_sec);

    event_base_loop(WORKER_CTX->evbase_thrd, EVLOOP_NO_EXIT_ON_EMPTY);

    return NULL;
}
