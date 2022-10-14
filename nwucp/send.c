#include <nwucp.h>

extern main_ctx_t *MAIN_CTX;
extern __thread worker_ctx_t *WORKER_CTX;

uint8_t create_eap_id()
{
	//srand((unsigned int)time(NULL));
	uint8_t id = rand();
	return id;
}

void eap_send_eap_request(ue_ctx_t *ue_ctx, int msg_id, const char *nas_pdu)
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
	if (res < 0) {
		fprintf(stderr, "%s() ue [%s] error! (%d:%s)\n", __func__, UE_TAG(ue_ctx), errno, strerror(errno));
	}
}

void eap_send_final_eap(ue_ctx_t *ue_ctx, bool success, const char *security_key)
{
	eap_relay_t *eap_5g = &ue_ctx->eap_5g;
	memset(eap_5g, 0x00, sizeof(eap_relay_t));

	/* make eap start message at ue_ctx */
	eap_5g->eap_code = success == true ? EAP_SUCCESS : EAP_FAILURE;
	eap_5g->eap_id = create_eap_id();

	/* make IKE_AUTH_RES */
	ike_msg_t msg = { .mtype = 1 }, *ike_msg = &msg;
	// ue_id part
	memcpy(&ike_msg->ike_tag, &ue_ctx->ike_tag, sizeof(ike_tag_t));
	// n3iwf_msg part
	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;
	n3iwf_msg->msg_code = N3_IKE_AUTH_RES;
	n3iwf_msg->res_code = success == true ? N3_EAP_SUCCESS : N3_EAP_FAILURE;
	memcpy(&n3iwf_msg->ctx_info, &ue_ctx->ctx_info, sizeof(ctx_info_t));
	// eap_5g_part
	memcpy(&ike_msg->eap_5g, eap_5g, sizeof(eap_relay_t));
	// n3_eap_result
	n3_eap_result_t *eap_result = &ike_msg->ike_data.eap_result;
	ike_msg->ike_choice	= choice_eap_result;
	sprintf(eap_result->tcp_server_ip, "%s", MAIN_CTX->tcp_server.tcp_listen_addr);
	eap_result->tcp_server_port = MAIN_CTX->tcp_server.tcp_listen_port;
	sprintf(eap_result->internal_ip, "%s", ue_ctx->ip_addr);
	eap_result->internal_netmask = MAIN_CTX->ue_info.netmask;
	sprintf(eap_result->security_key_str, "%64s", security_key);

	/* start timer */
	ue_ctx_stop_timer(ue_ctx);
	struct timeval tmout_sec = { N3_EXPIRE_TM_SEC, 0 };
	ue_ctx->ev_timer = event_new(WORKER_CTX->evbase_thrd, -1, EV_TIMEOUT, ue_ctx_release, ue_ctx);
	event_add(ue_ctx->ev_timer, &tmout_sec);

	/* send message to EAP5G */
	int res = msgsnd(MAIN_CTX->QID_INFO.nwucp_eap5g_qid, ike_msg, IKE_MSG_SIZE, IPC_NOWAIT);
	if (res < 0) {
		fprintf(stderr, "%s() ue [%s] error! (%d:%s)\n", __func__, UE_TAG(ue_ctx), errno, strerror(errno));
	}
}

void ike_send_pdu_release(ue_ctx_t *ue_ctx, n3_pdu_info_t *pdu_info)
{
	/* make IKE_AUTH_RES */
	ike_msg_t msg = { .mtype = 1 }, *ike_msg = &msg;
	// ue_id part
	memcpy(&ike_msg->ike_tag, &ue_ctx->ike_tag, sizeof(ike_tag_t));
	// n3iwf_msg part
	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;
	n3iwf_msg->msg_code = N3_IKE_INFORM_REQ;
	n3iwf_msg->res_code = N3_PDU_DELETE_REQUEST;
	memcpy(&n3iwf_msg->ctx_info, &ue_ctx->ctx_info, sizeof(ctx_info_t));
	// pdu_info part
	memcpy(&ike_msg->ike_data.pdu_info, pdu_info, sizeof(n3_pdu_info_t));
	ike_msg->ike_choice = choice_pdu_info;

	/* start timer */
	ue_ctx_stop_timer(ue_ctx);
	struct timeval tmout_sec = { N3_EXPIRE_TM_SEC, 0 };
	ue_ctx->ev_timer = event_new(WORKER_CTX->evbase_thrd, -1, EV_TIMEOUT, ue_ctx_release, ue_ctx);
	event_add(ue_ctx->ev_timer, &tmout_sec);

	/* send message to EAP5G */
	int res = msgsnd(MAIN_CTX->QID_INFO.nwucp_eap5g_qid, ike_msg, IKE_MSG_SIZE, IPC_NOWAIT);
	if (res < 0) {
		fprintf(stderr, "%s() ue [%s] error! (%d:%s)\n", __func__, UE_TAG(ue_ctx), errno, strerror(errno));
	}
}

void ike_send_pdu_request(ue_ctx_t *ue_ctx, n3_pdu_info_t *pdu_info)
{
	/* make IKE_AUTH_RES */
	ike_msg_t msg = { .mtype = 1 }, *ike_msg = &msg;
	// ue_id part
	memcpy(&ike_msg->ike_tag, &ue_ctx->ike_tag, sizeof(ike_tag_t));
	// n3iwf_msg part
	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;
	n3iwf_msg->msg_code = N3_CREATE_CHILD_SA_REQ;
	n3iwf_msg->res_code = N3_PDU_CREATE_REQUEST;
	memcpy(&n3iwf_msg->ctx_info, &ue_ctx->ctx_info, sizeof(ctx_info_t));
	// pdu_info part
	memcpy(&ike_msg->ike_data.pdu_info, pdu_info, sizeof(n3_pdu_info_t));
	ike_msg->ike_choice = choice_pdu_info;

	/* start timer */
	ue_ctx_stop_timer(ue_ctx);
	struct timeval tmout_sec = { N3_EXPIRE_TM_SEC, 0 };
	ue_ctx->ev_timer = event_new(WORKER_CTX->evbase_thrd, -1, EV_TIMEOUT, ue_ctx_release, ue_ctx);
	event_add(ue_ctx->ev_timer, &tmout_sec);

	/* send message to EAP5G */
	int res = msgsnd(MAIN_CTX->QID_INFO.nwucp_eap5g_qid, ike_msg, IKE_MSG_SIZE, IPC_NOWAIT);
	if (res < 0) {
		fprintf(stderr, "%s() ue [%s] error! (%d:%s)\n", __func__, UE_TAG(ue_ctx), errno, strerror(errno));
	}
}

void ike_send_ue_release(ue_ctx_t *ue_ctx)
{
	/* make IKE_AUTH_RES */
	ike_msg_t msg = { .mtype = 1 }, *ike_msg = &msg;
	// ue_id part
	memcpy(&ike_msg->ike_tag, &ue_ctx->ike_tag, sizeof(ike_tag_t));
	// n3iwf_msg part
	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;
	n3iwf_msg->msg_code = N3_IKE_INFORM_REQ;
	n3iwf_msg->res_code = N3_IPSEC_DELETE_UE_CTX;
	memcpy(&n3iwf_msg->ctx_info, &ue_ctx->ctx_info, sizeof(ctx_info_t));

	/* start timer */
	ue_ctx_stop_timer(ue_ctx);
	struct timeval tmout_sec = { N3_EXPIRE_TM_SEC, 0 };
	ue_ctx->ev_timer = event_new(WORKER_CTX->evbase_thrd, -1, EV_TIMEOUT, ue_ctx_release, ue_ctx);
	event_add(ue_ctx->ev_timer, &tmout_sec);

	/* send message to EAP5G */
	int res = msgsnd(MAIN_CTX->QID_INFO.nwucp_eap5g_qid, ike_msg, IKE_MSG_SIZE, IPC_NOWAIT);
	if (res < 0) {
		fprintf(stderr, "%s() ue [%s] error! (%d:%s)\n", __func__, UE_TAG(ue_ctx), errno, strerror(errno));
	}
}

void ngap_send_uplink_nas(ue_ctx_t *ue_ctx, char *nas_str)
{
	json_object *js_uplink_nas_transport_message = 
		create_uplink_nas_transport_json(ue_ctx->amf_tag.amf_ue_id, ue_ctx->amf_tag.ran_ue_id, nas_str, ue_ctx->ike_tag.ue_from_addr, ue_ctx->ike_tag.ue_from_port);

	/* start timer */
	ue_ctx_stop_timer(ue_ctx);

	if (ue_ctx->ipsec_sa_created == 0) {
		struct timeval tmout_sec = { N3_EXPIRE_TM_SEC, 0 };
		ue_ctx->ev_timer = event_new(WORKER_CTX->evbase_thrd, -1, EV_TIMEOUT, ue_ctx_release, ue_ctx);
		event_add(ue_ctx->ev_timer, &tmout_sec);
	}

	ue_ctx_transit_state(ue_ctx, "N2 Uplink NAS transport");

	/* send to amf */
	ngap_send_json(ue_ctx->amf_tag.amf_host, js_uplink_nas_transport_message);

	/* release resource */
	json_object_put(js_uplink_nas_transport_message);
}

void tcp_save_downlink_nas(ue_ctx_t *ue_ctx, const char *nas_pdu)
{
	if (nas_pdu != NULL) {
		ue_ctx->temp_cached_nas_message = g_slist_append(ue_ctx->temp_cached_nas_message, strdup(nas_pdu));
	}
}

void tcp_send_downlink_nas(ue_ctx_t *ue_ctx, const char *nas_pdu)
{
	if (nas_pdu != NULL) {
		ue_ctx->temp_cached_nas_message = g_slist_append(ue_ctx->temp_cached_nas_message, strdup(nas_pdu));
	}

	if (ue_ctx->sock_ctx != NULL) {
		sock_flush_cb(ue_ctx);
	} else {
		fprintf(stderr, "%s() ue [%s] save temp_cache_nas_message\n", __func__, UE_TAG(ue_ctx));
	}
}

