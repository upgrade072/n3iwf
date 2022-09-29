#include <nwucp.h>

extern main_ctx_t *MAIN_CTX;
extern __thread worker_ctx_t *WORKER_CTX;

void eap_proc_5g_start(int conn_fd, short events, void *data)
{
	ue_ctx_t *ue_ctx = (ue_ctx_t *)data;

	ue_ctx_transit_state(ue_ctx, "EAP_REQ_5G_START");
	eap_send_eap_request(ue_ctx, NAS_5GS_START, NULL); 
}

void eap_proc_5g_nas(ue_ctx_t *ue_ctx, const char *nas_pdu)
{
	ue_ctx_transit_state(ue_ctx, "EAP_REQ_5G_NAS");
	eap_send_eap_request(ue_ctx, NAS_5GS_NAS, nas_pdu); 
}

void eap_proc_final(ue_ctx_t *ue_ctx, bool success, const char *security_key)
{
	ue_ctx_transit_state(ue_ctx, success == true ? "UE_REGI_SUCCESS" : "UE_REGI_FAIL");
	eap_send_final_eap(ue_ctx, success, security_key);
}

void tcp_proc_downlink_nas(ue_ctx_t *ue_ctx, const char *nas_pdu)
{
	ue_ctx_transit_state(ue_ctx, "TCP_DOWNLINK_NAS");
	tcp_send_downlink_nas(ue_ctx, nas_pdu); 
}

void ike_proc_pdu_request(ue_ctx_t *ue_ctx, n3_pdu_info_t *pdu_info)
{
	ue_ctx_transit_state(ue_ctx, "PDU_SESSION_REQUEST");
	ike_send_pdu_request(ue_ctx, pdu_info);
}

void ike_proc_inform_request(ue_ctx_t *ue_ctx, int res_code)
{
	ue_ctx_transit_state(ue_ctx, "INFORM_REQUEST");
	ike_send_inform_request(ue_ctx, res_code);
}

void ngap_proc_initial_ue_message(ue_ctx_t *ue_ctx, ike_msg_t *ike_msg)
{
	eap_relay_t *eap_recv = &ike_msg->eap_5g;

	if (eap_recv->eap_code != EAP_RESPONSE && eap_recv->msg_id != NAS_5GS_NAS) {
		fprintf(stderr, "{todo} %s() recv invalid eap_id or msg_id!\n", __func__);
		goto NPIUM_ERR;
	}

	if (strlen(ike_msg->ike_tag.cp_amf_host) == 0) {
		fprintf(stderr, "{todo} %s() recv invalid amf_select!\n", __func__);
		goto NPIUM_ERR;
	} else {
		sprintf(ue_ctx->amf_tag.amf_host, "%s", ike_msg->ike_tag.cp_amf_host);
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

	json_object *js_initial_ue_message = create_initial_ue_message_json(ue_ctx->index, eap_recv->nas_str, ue_ctx->ike_tag.ue_from_addr, ue_ctx->ike_tag.ue_from_port, eap_recv->an_param.cause.cause_str);

	/* start timer */
	ue_ctx_stop_timer(ue_ctx);
	struct timeval tmout_sec = { N3_EXPIRE_TM_SEC, 0 };
	ue_ctx->ev_timer = event_new(WORKER_CTX->evbase_thrd, -1, EV_TIMEOUT, ue_ctx_release, ue_ctx);
	event_add(ue_ctx->ev_timer, &tmout_sec);

	ue_ctx_transit_state(ue_ctx, "NGAP_REQ_REGI");

	/* send to amf */
	ngap_send_json(ue_ctx->amf_tag.amf_host, js_initial_ue_message);

	/* release resource */
	json_object_put(js_initial_ue_message);

	return;

NPIUM_ERR:
	return ue_ctx_unset(ue_ctx);
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

void ngap_proc_initial_context_setup_response(ue_ctx_t *ue_ctx, ike_msg_t *ike_msg)
{
	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;

	ue_ctx->ipsec_sa_created = 
		n3iwf_msg->res_code == N3_IPSEC_CREATE_SUCCESS ? 1 : 0;

	json_object *js_initial_context_setup_response_message = 
		create_initial_context_setup_response_json(
				n3iwf_msg->res_code == N3_IPSEC_CREATE_SUCCESS ? "successfulOutcome" : "UnSuccessfulOutcome", 
				ue_ctx->amf_tag.amf_ue_id, ue_ctx->amf_tag.ran_ue_id);

	/* start timer */
	ue_ctx_stop_timer(ue_ctx);
#if 0
	// TODO : if ipsec fail release ue_ctx
	struct timeval tmout_sec = { N3_EXPIRE_TM_SEC, 0 };
	ue_ctx->ev_timer = event_new(WORKER_CTX->evbase_thrd, -1, EV_TIMEOUT, ue_ctx_release, ue_ctx);
	event_add(ue_ctx->ev_timer, &tmout_sec);
#endif

	ue_ctx_transit_state(ue_ctx, "NGAP_INIT_CTX_SETUP_RSP");

	/* send to amf */
	ngap_send_json(ue_ctx->amf_tag.amf_host, js_initial_context_setup_response_message);

	/* release resource */
	json_object_put(js_initial_context_setup_response_message);
}

void ngap_proc_pdu_session_resource_setup_response(ue_ctx_t *ue_ctx, ike_msg_t *ike_msg)
{
	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;
	n3_pdu_info_t *pdu_info = &ike_msg->ike_data.pdu_info;

	json_object *js_pdu_session_resouce_setup_response = 
		create_pdu_session_resource_setup_response_json(
				n3iwf_msg->res_code == N3_PDU_CREATE_SUCCESS ? "successfulOutcome" : "UnSuccessfulOutcome", 
				ue_ctx->amf_tag.amf_ue_id, ue_ctx->amf_tag.ran_ue_id, pdu_info);

	ue_ctx_transit_state(ue_ctx, n3iwf_msg->res_code == N3_PDU_CREATE_SUCCESS ? 
			"PDU_SESSION_CRT_SUCCESS" : "PDU_SESSION_CRT_FAIL");

	/* send to amf */
	ngap_send_json(ue_ctx->amf_tag.amf_host, js_pdu_session_resouce_setup_response);

	/* release resource */
	json_object_put(js_pdu_session_resouce_setup_response);
}

void ngap_proc_ue_context_release_complete(ue_ctx_t *ue_ctx)
{
	json_object *js_ue_context_release_complete = 
		create_ue_context_release_complete_json(ue_ctx->amf_tag.amf_ue_id, ue_ctx->amf_tag.ran_ue_id, 
				ue_ctx->ike_tag.ue_from_addr, ue_ctx->ike_tag.ue_from_port, ue_ctx);

	/* send to amf */
	ngap_send_json(ue_ctx->amf_tag.amf_host, js_ue_context_release_complete);

	/* release resource */
	json_object_put(js_ue_context_release_complete);

	/* final release ue ctx */
	event_base_once(WORKER_CTX->evbase_thrd, -1, EV_TIMEOUT, ue_ctx_release, ue_ctx, NULL);
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
	} else {
		return ngap_proc_uplink_nas_transport(ue_ctx, ike_msg);
	}
}

void nas_regi_to_amf(ike_msg_t *ike_msg)
{
	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;
	ue_ctx_t *ue_ctx = ue_ctx_get_by_index(n3iwf_msg->ctx_info.cp_id, WORKER_CTX);

	if (ue_ctx == NULL) {
		fprintf(stderr, "{todo} %s() called null ue_ctx! reply to UP fail response\n", __func__);
		return;
	}
	ue_ctx_stop_timer(ue_ctx);

	return ngap_proc_initial_context_setup_response(ue_ctx, ike_msg);
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
	if (ue_check_ngap_id(ue_ctx, js_ngap_pdu) < 0) {
		fprintf(stderr, "%s() fail cause ngap_id not exist!\n", __func__);
		goto NRTU_ERR;
	}

	const char *nas_pdu = ngap_get_nas_pdu(js_ngap_pdu);
	if (nas_pdu == NULL) {
		fprintf(stderr, "{todo} %s() fail to get mandatory nas_pdu!\n", __func__);
		goto NRTU_ERR;
	}

	if (ue_ctx->ipsec_sa_created == 0) {
		/* Signalling SA not yet created */
		return eap_proc_5g_nas(ue_ctx, nas_pdu);
	} else {
		/* Signalling SA already created */
		return tcp_proc_downlink_nas(ue_ctx, nas_pdu);
	}

NRTU_ERR:
	if (ue_ctx != NULL) {
		ue_ctx_unset(ue_ctx);
	}
}

void ue_regi_res_handle(ngap_msg_t *ngap_msg, json_object *js_ngap_pdu)
{
    ue_ctx_t *ue_ctx = ue_ctx_get_by_index(ngap_msg->ngap_tag.id, WORKER_CTX);
    if (ue_ctx == NULL) {
        fprintf(stderr, "{todo} %s() fail to get ue (index:%d)\n", __func__, ngap_msg->ngap_tag.id);
        goto URRH_ERR;
    }
    
    /* stop timer */
    ue_ctx_stop_timer(ue_ctx);
    
    /* check ngap message, have mandatory */
	if (ue_check_ngap_id(ue_ctx, js_ngap_pdu) < 0) {
		fprintf(stderr, "%s() fail cause ngap_id not exist!\n", __func__);
		goto URRH_ERR;
	}

	const char *security_key = ngap_get_security_key(js_ngap_pdu);
    if (security_key == NULL) {
		fprintf(stderr, "%s() fail cause security_key not exist!\n", __func__);
        goto URRH_ERR;
    }
	if (ue_ctx->security_key != NULL) {
		free(ue_ctx->security_key);
		ue_ctx->security_key = NULL;
	}
	ue_ctx->security_key = strdup(security_key);

	// TODO if pduSessionResourceSetupListCtx exist
	// TODO if nas pdu exist

	if (ue_ctx->js_ue_regi_data != NULL) {
		fprintf(stderr, "{dbg} %s() check UE have old regi data, will replaced with new!\n", __func__);
		json_object_put(ue_ctx->js_ue_regi_data);
		ue_ctx->js_ue_regi_data = NULL;
	}
	json_object_deep_copy(js_ngap_pdu, &ue_ctx->js_ue_regi_data, NULL);

	eap_proc_final(ue_ctx, true, security_key);

	return;

URRH_ERR:
	eap_proc_final(ue_ctx, false, NULL);

	if (ue_ctx != NULL) {
		ue_ctx_unset(ue_ctx);
	}
}

void ue_pdu_setup_req_handle(ngap_msg_t *ngap_msg, json_object *js_ngap_pdu)
{
    ue_ctx_t *ue_ctx = ue_ctx_get_by_index(ngap_msg->ngap_tag.id, WORKER_CTX);
    if (ue_ctx == NULL) {
        fprintf(stderr, "{todo} %s() fail to get ue (index:%d)\n", __func__, ngap_msg->ngap_tag.id);
        goto UPSH_ERR;
    }
    
    /* stop timer */
    ue_ctx_stop_timer(ue_ctx);

    /* check ngap message, have mandatory */
	if (ue_check_ngap_id(ue_ctx, js_ngap_pdu) < 0) {
		fprintf(stderr, "%s() fail cause ngap_id not exist!\n", __func__);
		goto UPSH_ERR;
	}

	n3_pdu_info_t pdu_buffer = {0,}, *pdu_info = &pdu_buffer;

	pdu_info->ue_ambr_dl = ngap_get_ue_ambr_dl(js_ngap_pdu);
	pdu_info->ue_ambr_ul = ngap_get_ue_ambr_ul(js_ngap_pdu);
	if (pdu_info->ue_ambr_dl < 0 || pdu_info->ue_ambr_ul < 0) {
		fprintf(stderr, "%s() fail cause ue_ambr not exist!\n", __func__);
		goto UPSH_ERR;
	}

	key_list_t key_pdu_session_resource_setup_list_su_req = {0,};
	json_object *js_pdu_session_resource_setup_list_su_req = 
		search_json_object_ex(js_ngap_pdu, "/initiatingMessage/value/protocolIEs/{id:74, value}/", &key_pdu_session_resource_setup_list_su_req);

	if (js_pdu_session_resource_setup_list_su_req == NULL) {
        fprintf(stderr, "{todo} %s() fail cause pdu_session_resource_setup_list_su_req not exist!\n", __func__);
        goto UPSH_ERR;
	}

	pdu_info->pdu_num = 
		pdu_proc_fill_pdu_sess_setup_list(ue_ctx, js_pdu_session_resource_setup_list_su_req, pdu_info->pdu_sessions);

	ike_proc_pdu_request(ue_ctx, pdu_info);

	return;

UPSH_ERR:
	fprintf(stderr, "{todo} %s() must to something!\n", __func__);

	if (ue_ctx != NULL) {
		ue_ctx_unset(ue_ctx);
	}
}

void ue_pdu_setup_res_handle(ike_msg_t *ike_msg)
{
	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;
	ue_ctx_t *ue_ctx = ue_ctx_get_by_index(n3iwf_msg->ctx_info.cp_id, WORKER_CTX);

	if (ue_ctx == NULL) {
		fprintf(stderr, "{todo} %s() called null ue_ctx! reply to UP fail response\n", __func__);
		return;
	}
	ue_ctx_stop_timer(ue_ctx);

	if (n3iwf_msg->res_code == N3_PDU_CREATE_SUCCESS) {
		/* send tcp-nas-pdu_accept */
		g_slist_foreach(ue_ctx->pdu_ctx_list, (GFunc)pdu_proc_sess_establish_accept, ue_ctx);
	} else {
		pdu_proc_flush_ctx(ue_ctx);
	}

	return ngap_proc_pdu_session_resource_setup_response(ue_ctx, ike_msg);
}

void ue_ctx_release_handle(ngap_msg_t *ngap_msg, json_object *js_ngap_pdu)
{
    ue_ctx_t *ue_ctx = ue_ctx_get_by_index(ngap_msg->ngap_tag.id, WORKER_CTX);
    if (ue_ctx == NULL) {
        fprintf(stderr, "{todo} %s() fail to get ue (index:%d)\n", __func__, ngap_msg->ngap_tag.id);
        goto UCRH_ERR;
    }
    
    /* stop timer */
    ue_ctx_stop_timer(ue_ctx);

    uint32_t amf_ue_ngap_id = ngap_get_ctx_rel_amf_ue_ngap_id(js_ngap_pdu);
    uint32_t ran_ue_ngap_id = ngap_get_ctx_rel_ran_ue_ngap_id(js_ngap_pdu);
	const char *rel_cause = ngap_get_ctx_rel_cause(js_ngap_pdu);

	if (amf_ue_ngap_id != ue_ctx->amf_tag.amf_ue_id ||
		ran_ue_ngap_id != ue_ctx->amf_tag.ran_ue_id ||
		rel_cause == NULL) {
		fprintf(stderr, "{dbg} %s check mandatory fail!\n", __func__);
		goto UCRH_ERR;
	}

	ike_proc_inform_request(ue_ctx, N3_IPSEC_DELETE_PAYLOAD);

	return;

UCRH_ERR:
	return;
}

void ue_inform_res_handle(ike_msg_t *ike_msg)
{
	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;
	ue_ctx_t *ue_ctx = ue_ctx_get_by_index(n3iwf_msg->ctx_info.cp_id, WORKER_CTX);

	if (ue_ctx == NULL) {
		fprintf(stderr, "{todo} %s() called null ue_ctx! reply to UP fail response\n", __func__);
		return;
	}
	ue_ctx_stop_timer(ue_ctx);

	switch (n3iwf_msg->res_code) {
		case N3_IPSEC_DELETE_PAYLOAD:
			return ngap_proc_ue_context_release_complete(ue_ctx);
	}
}
