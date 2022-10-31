#include <nwucp.h>

extern main_ctx_t *MAIN_CTX;
extern __thread worker_ctx_t *WORKER_CTX;

void eap_proc_5g_start(int conn_fd, short events, void *data)
{
	ue_ctx_t *ue_ctx = (ue_ctx_t *)data;

	ue_ctx_transit_state(ue_ctx, "EAP-Req/5G-Start");
	eap_send_eap_request(ue_ctx, NAS_5GS_START, NULL); 
}

void eap_proc_5g_nas(ue_ctx_t *ue_ctx, const char *nas_pdu)
{
	ue_ctx_transit_state(ue_ctx, "EAP-Req/5G-NAS");
	eap_send_eap_request(ue_ctx, NAS_5GS_NAS, nas_pdu); 
}

void eap_proc_final(ue_ctx_t *ue_ctx, bool success, const char *security_key)
{
	ue_ctx_transit_state(ue_ctx, success == true ? "EAP-Success" : "EAP-Failure");
	eap_send_final_eap(ue_ctx, success, security_key);
}

void tcp_proc_downlink_nas(ue_ctx_t *ue_ctx, const char *nas_pdu)
{
	ue_ctx_transit_state(ue_ctx, "N2 Downlink NAS transport");
	tcp_send_downlink_nas(ue_ctx, nas_pdu); 
}

void ike_proc_pdu_release(ue_ctx_t *ue_ctx, n3_pdu_info_t *pdu_info)
{
	ue_ctx_transit_state(ue_ctx, "N2 PDU Session Release");
	ike_send_pdu_release(ue_ctx, pdu_info);
}

void ike_proc_pdu_request(ue_ctx_t *ue_ctx, n3_pdu_info_t *pdu_info)
{
	ue_ctx_transit_state(ue_ctx, "N3 PDU Session Request");
	ike_send_pdu_request(ue_ctx, pdu_info);
}

void ike_proc_ue_release(ue_ctx_t *ue_ctx)
{
	ue_ctx_transit_state(ue_ctx, "N2 UE Context Release");
	ike_send_ue_release(ue_ctx);
}

int ngap_proc_initial_ue_message(ue_ctx_t *ue_ctx, ike_msg_t *ike_msg)
{
	eap_relay_t *eap_recv = &ike_msg->eap_5g;

	if (eap_recv->eap_code != EAP_RESPONSE && eap_recv->msg_id != NAS_5GS_NAS) {
		fprintf(stderr, "TODO %s() ue [%s] recv invalid eap_id or msg_id!\n", __func__, UE_TAG(ue_ctx));
		goto NPIUM_ERR;
	}

	if (strlen(ike_msg->ike_tag.cp_amf_host) == 0) {
		fprintf(stderr, "TODO %s() ue [%s] recv invalid amf_select!\n", __func__, UE_TAG(ue_ctx));
		goto NPIUM_ERR;
	} else {
		sprintf(ue_ctx->amf_tag.amf_host, "%s", ike_msg->ike_tag.cp_amf_host);
	}

	if (strlen(eap_recv->nas_str) == 0) {
		fprintf(stderr, "TODO %s() ue [%s] recv invalid nas_pdu!\n", __func__, UE_TAG(ue_ctx));
		goto NPIUM_ERR;
	}

	if (eap_recv->an_param.cause.set == 0) {
		fprintf(stderr, "TODO %s() ue [%s] recv invalid cause!\n", __func__, UE_TAG(ue_ctx));
		goto NPIUM_ERR;
	}

	fprintf(stderr, "%s() ue [%s] save amf host [%s],\n\t recv nas_pdu [%s], recv cause [%s]\n", 
			__func__, UE_TAG(ue_ctx), ue_ctx->amf_tag.amf_host, ike_msg->eap_5g.nas_str, eap_recv->an_param.cause.cause_str);

	json_object *js_initial_ue_message = create_initial_ue_message_json(ue_ctx->index, eap_recv->nas_str, ue_ctx->ike_tag.ue_from_addr, ue_ctx->ike_tag.ue_from_port, eap_recv->an_param.cause.cause_str);

	/* start timer */
	ue_ctx_stop_timer(ue_ctx);
	struct timeval tmout_sec = { N3_EXPIRE_TM_SEC, 0 };
	ue_ctx->ev_timer = event_new(WORKER_CTX->evbase_thrd, -1, EV_TIMEOUT, ue_ctx_release, ue_ctx);
	event_add(ue_ctx->ev_timer, &tmout_sec);

	ue_ctx_transit_state(ue_ctx, "N2 msg (Registration Request)");

	/* send to amf */
	ngap_send_json(ue_ctx->amf_tag.amf_host, ue_ctx, js_initial_ue_message);

	/* release resource */
	json_object_put(js_initial_ue_message);

	return 0;

NPIUM_ERR:
	ue_ctx_unset(ue_ctx);

	return -1;
}

int ngap_proc_uplink_nas_transport(ue_ctx_t *ue_ctx, ike_msg_t *ike_msg)
{
	eap_relay_t *eap_recv = &ike_msg->eap_5g;

	if (eap_recv->eap_code != EAP_RESPONSE && eap_recv->msg_id != NAS_5GS_NAS) {
		fprintf(stderr, "TODO %s() ue [%s] recv invalid eap_id or msg_id!\n", __func__, UE_TAG(ue_ctx));
		goto NPUNT_ERR;
	}

	if (strlen(eap_recv->nas_str) == 0) {
		fprintf(stderr, "TODO %s() ue [%s] recv invalid nas_pdu!\n", __func__, UE_TAG(ue_ctx));
		goto NPUNT_ERR;
	}

	fprintf(stderr, "%s() ue [%s],\n\t recv nas_pdu [%s]\n", __func__, UE_TAG(ue_ctx), eap_recv->nas_str);

	ngap_send_uplink_nas(ue_ctx, ike_msg->eap_5g.nas_str);

	return 0;

NPUNT_ERR:
	ue_ctx_unset(ue_ctx);

	return -1;
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

	ue_ctx_transit_state(ue_ctx, "Initial Context Setup Res");

	/* send to amf */
	ngap_send_json(ue_ctx->amf_tag.amf_host, ue_ctx, js_initial_context_setup_response_message);

	/* release resource */
	json_object_put(js_initial_context_setup_response_message);
}

void ngap_proc_pdu_session_resource_release_response(ue_ctx_t *ue_ctx, ike_msg_t *ike_msg)
{
	n3iwf_msg_t *n3iwf_msg	= &ike_msg->n3iwf_msg;
	n3_pdu_info_t *pdu_info	= &ike_msg->ike_data.pdu_info;
	ike_msg->ike_choice		= choice_pdu_info;

	json_object *js_pdu_session_resouce_release_response = 
		create_pdu_session_resource_release_response_json(
				n3iwf_msg->res_code == N3_PDU_DELETE_SUCCESS ? "successfulOutcome" : "UnSuccessfulOutcome", 
				ue_ctx->amf_tag.amf_ue_id, ue_ctx->amf_tag.ran_ue_id, pdu_info);

	ue_ctx_transit_state(ue_ctx, n3iwf_msg->res_code == N3_PDU_DELETE_SUCCESS ? 
			"PDU Session Release Success" : "PDU Session Release Failure");

	/* send to amf */
	ngap_send_json(ue_ctx->amf_tag.amf_host, ue_ctx, js_pdu_session_resouce_release_response);

	/* release resource */
	json_object_put(js_pdu_session_resouce_release_response);

	/* send nas message (pdu session release) command */
	tcp_proc_downlink_nas(ue_ctx, NULL);

	/* remove pdu ctx */
	pdu_proc_remove_ctx(ue_ctx, pdu_info);

}

void ngap_proc_pdu_session_resource_setup_response(ue_ctx_t *ue_ctx, ike_msg_t *ike_msg)
{
	n3iwf_msg_t *n3iwf_msg	= &ike_msg->n3iwf_msg;
	n3_pdu_info_t *pdu_info = &ike_msg->ike_data.pdu_info;
	ike_msg->ike_choice		= choice_pdu_info;

	json_object *js_pdu_session_resouce_setup_response = 
		create_pdu_session_resource_setup_response_json(
				n3iwf_msg->res_code == N3_PDU_CREATE_SUCCESS ? "successfulOutcome" : "UnSuccessfulOutcome", 
				ue_ctx->amf_tag.amf_ue_id, ue_ctx->amf_tag.ran_ue_id, pdu_info);

	ue_ctx_transit_state(ue_ctx, n3iwf_msg->res_code == N3_PDU_CREATE_SUCCESS ? 
			"PDU Session Create Success" : "PDU Session Create Failure");

	/* send to amf */
	ngap_send_json(ue_ctx->amf_tag.amf_host, ue_ctx, js_pdu_session_resouce_setup_response);

	/* release resource */
	json_object_put(js_pdu_session_resouce_setup_response);
}

void ngap_proc_ue_context_release_request(ue_ctx_t *ue_ctx)
{
	json_object *js_ue_context_release_request = 
		create_ue_context_release_request_json(ue_ctx->amf_tag.amf_ue_id, ue_ctx->amf_tag.ran_ue_id, ue_ctx);

	/* send to amf */
	ngap_send_json(ue_ctx->amf_tag.amf_host, ue_ctx, js_ue_context_release_request);

	/* release resource */
	json_object_put(js_ue_context_release_request);
}

void ngap_proc_ue_context_release_complete(ue_ctx_t *ue_ctx)
{
	json_object *js_ue_context_release_complete = 
		create_ue_context_release_complete_json(ue_ctx->amf_tag.amf_ue_id, ue_ctx->amf_tag.ran_ue_id, 
				ue_ctx->ike_tag.ue_from_addr, ue_ctx->ike_tag.ue_from_port, ue_ctx);

	/* send to amf */
	ngap_send_json(ue_ctx->amf_tag.amf_host, ue_ctx, js_ue_context_release_complete);

	/* release resource */
	json_object_put(js_ue_context_release_complete);

	/* final release ue ctx */
	event_base_once(WORKER_CTX->evbase_thrd, -1, EV_TIMEOUT, ue_ctx_release, ue_ctx, NULL);
}

int nas_relay_to_amf(ike_msg_t *ike_msg)
{
	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;
	ue_ctx_t *ue_ctx = ue_ctx_get_by_index(n3iwf_msg->ctx_info.cp_id, WORKER_CTX);
	eap_relay_t *eap_5g = &ike_msg->eap_5g;

	if (ue_ctx == NULL) {
		fprintf(stderr, "TODO %s() called null ue_ctx!\n", __func__);
		return -1;
	}
	ue_ctx_stop_timer(ue_ctx);

	/* check eap_id */
	eap_relay_t *eap_send = &ue_ctx->eap_5g;
	eap_relay_t *eap_recv = &ike_msg->eap_5g;

	if (eap_send->eap_id != eap_recv->eap_id) {
		fprintf(stderr, "TODO %s() ue [%s] check eap_id mismatch send=(%d)/recv=(%d)!\n", 
				__func__, UE_TAG(ue_ctx), eap_send->eap_id, eap_recv->eap_id);
		ue_ctx_unset(ue_ctx);
		return -1;
	}

	if (eap_5g->an_param.set == RS_AN_PARAM_PROCESS) {
		return ngap_proc_initial_ue_message(ue_ctx, ike_msg);
	} else {
		return ngap_proc_uplink_nas_transport(ue_ctx, ike_msg);
	}
}

int nas_regi_to_amf(ike_msg_t *ike_msg)
{
	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;
	ue_ctx_t *ue_ctx = ue_ctx_get_by_index(n3iwf_msg->ctx_info.cp_id, WORKER_CTX);

	if (ue_ctx == NULL) {
		fprintf(stderr, "TODO %s() called null ue_ctx!\n", __func__);
		return -1;
	}
	ue_ctx_stop_timer(ue_ctx);

	ngap_proc_initial_context_setup_response(ue_ctx, ike_msg);

	return 0;
}

void ue_save_sctp_tag(ngap_msg_t *ngap_msg, json_object *js_ngap_pdu, event_caller_t caller)
{
	ue_ctx_t *ue_ctx = NULL;

	if (caller != EC_WORKER) {
		return;
	}
	if ((ue_ctx = ue_ctx_get_by_index(ngap_msg->ngap_tag.id, WORKER_CTX)) == NULL) {
		return;
	}
	sctp_tag_t *sctp_tag = &ngap_msg->sctp_tag;
	memcpy(&ue_ctx->sctp_tag, sctp_tag, sizeof(sctp_tag_t));
	NWUCP_TRACE(ue_ctx, DIR_AMF_TO_ME, js_ngap_pdu, NULL);
}

int nas_relay_to_ue(ngap_msg_t *ngap_msg, json_object *js_ngap_pdu)
{
	ue_ctx_t *ue_ctx = ue_ctx_get_by_index(ngap_msg->ngap_tag.id, WORKER_CTX);
	if (ue_ctx == NULL) {
		fprintf(stderr, "TODO %s() called null ue_ctx!\n", __func__);
		goto NRTU_ERR;
	}

	/* stop timer */
	ue_ctx_stop_timer(ue_ctx);

    /* check ngap message, have mandatory */
	if (ue_check_ngap_id(ue_ctx, js_ngap_pdu) < 0) {
		fprintf(stderr, "TODO %s() ue [%s]  fail, to get mandatory ngap_id!\n", __func__, UE_TAG(ue_ctx));
		goto NRTU_ERR;
	}

	const char *nas_pdu = ngap_get_nas_pdu(js_ngap_pdu);
	if (nas_pdu == NULL) {
		fprintf(stderr, "TODO %s() ue [%s] fail, to get mandatory nas_pdu!\n", __func__, UE_TAG(ue_ctx));
		goto NRTU_ERR;
	}

	if (ue_ctx->ipsec_sa_created == 0) {
		/* Signalling SA not yet created */
		eap_proc_5g_nas(ue_ctx, nas_pdu);
	} else {
		/* Signalling SA already created */
		tcp_proc_downlink_nas(ue_ctx, nas_pdu);
	}

	return 0;

NRTU_ERR:
	if (ue_ctx != NULL) {
		ue_ctx_unset(ue_ctx);
	}

	return -1;
}

int ue_context_release(ike_msg_t *ike_msg)
{
	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;
	ue_ctx_t *ue_ctx = ue_ctx_get_by_index(n3iwf_msg->ctx_info.cp_id, WORKER_CTX);

	if (ue_ctx == NULL) {
		fprintf(stderr, "TODO %s() called null ue_ctx!\n", __func__);
		return -1;
	}
	ue_ctx_stop_timer(ue_ctx);

	ngap_proc_ue_context_release_request(ue_ctx);

	return 0;
}

int ue_regi_res_handle(ngap_msg_t *ngap_msg, json_object *js_ngap_pdu)
{
    ue_ctx_t *ue_ctx = ue_ctx_get_by_index(ngap_msg->ngap_tag.id, WORKER_CTX);
    if (ue_ctx == NULL) {
		fprintf(stderr, "TODO %s() called null ue_ctx!\n", __func__);
        goto URRH_ERR;
	}
    
    /* stop timer */
    ue_ctx_stop_timer(ue_ctx);
    
    /* check ngap message, have mandatory */
	if (ue_check_ngap_id(ue_ctx, js_ngap_pdu) < 0) {
		fprintf(stderr, "TODO %s() ue [%s]  fail, to get mandatory ngap_id!\n", __func__, UE_TAG(ue_ctx));
		goto URRH_ERR;
	}

	const char *security_key = ngap_get_security_key(js_ngap_pdu);
    if (security_key == NULL) {
		fprintf(stderr, "TODO %s() ue [%s]  fail, to get mandatory security_key!\n", __func__, UE_TAG(ue_ctx));
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
		fprintf(stderr, "%s() ue [%s] check UE have old regi data, will replaced with new!\n", __func__, UE_TAG(ue_ctx));
		json_object_put(ue_ctx->js_ue_regi_data);
		ue_ctx->js_ue_regi_data = NULL;
	}
	json_object_deep_copy(js_ngap_pdu, &ue_ctx->js_ue_regi_data, NULL);

	eap_proc_final(ue_ctx, true, security_key);

	return 0;

URRH_ERR:
	eap_proc_final(ue_ctx, false, NULL);

	if (ue_ctx != NULL) {
		ue_ctx_unset(ue_ctx);
	}

	return -1;
}

int ue_pdu_release_req_handle(ngap_msg_t *ngap_msg, json_object *js_ngap_pdu)
{
    ue_ctx_t *ue_ctx = ue_ctx_get_by_index(ngap_msg->ngap_tag.id, WORKER_CTX);
    if (ue_ctx == NULL) {
		fprintf(stderr, "TODO %s() called null ue_ctx!\n", __func__);
        goto UPRR_ERR;
	}
    
    /* stop timer */
    ue_ctx_stop_timer(ue_ctx);

    /* check ngap message, have mandatory */
	if (ue_check_ngap_id(ue_ctx, js_ngap_pdu) < 0) {
		fprintf(stderr, "TODO %s() ue [%s]  fail, to get mandatory ngap_id!\n", __func__, UE_TAG(ue_ctx));
		goto UPRR_ERR;
	}

	const char *nas_pdu = ngap_get_nas_pdu(js_ngap_pdu);
	if (nas_pdu == NULL) {
		fprintf(stderr, "TODO %s() ue [%s] fail, to get mandatory nas_pdu!\n", __func__, UE_TAG(ue_ctx));
		goto UPRR_ERR;
	}

	n3_pdu_info_t pdu_buffer = {0,}, *pdu_info = &pdu_buffer;

	key_list_t key_pdu_session_resource_to_release_list_rel_cmd = {0,};
	json_object *js_pdu_session_resource_to_release_list_rel_cmd = 
		search_json_object_ex(js_ngap_pdu, "/initiatingMessage/value/protocolIEs/{id:79, value}/", &key_pdu_session_resource_to_release_list_rel_cmd);

	if (js_pdu_session_resource_to_release_list_rel_cmd == NULL) {
		fprintf(stderr, "TODO %s() ue [%s]  fail, to get mandatory pdu_session_resource_to_release_list_rel_cmd!\n", __func__, UE_TAG(ue_ctx));
        goto UPRR_ERR;
	}

	tcp_save_downlink_nas(ue_ctx, nas_pdu);

	pdu_info->pdu_num = 
		pdu_proc_fill_pdu_sess_release_list(ue_ctx, js_pdu_session_resource_to_release_list_rel_cmd, pdu_info->pdu_sessions);

	ike_proc_pdu_release(ue_ctx, pdu_info);

	return 0;

UPRR_ERR:
	if (ue_ctx != NULL) {
		ue_ctx_unset(ue_ctx);
	}

	return -1;
}

int ue_pdu_setup_req_handle(ngap_msg_t *ngap_msg, json_object *js_ngap_pdu)
{
    ue_ctx_t *ue_ctx = ue_ctx_get_by_index(ngap_msg->ngap_tag.id, WORKER_CTX);
    if (ue_ctx == NULL) {
		fprintf(stderr, "TODO %s() called null ue_ctx!\n", __func__);
        goto UPSH_ERR;
	}

    /* stop timer */
    ue_ctx_stop_timer(ue_ctx);

    /* check ngap message, have mandatory */
	if (ue_check_ngap_id(ue_ctx, js_ngap_pdu) < 0) {
		fprintf(stderr, "TODO %s() ue [%s]  fail, to get mandatory ngap_id!\n", __func__, UE_TAG(ue_ctx));
		goto UPSH_ERR;
	}

	n3_pdu_info_t pdu_buffer = {0,}, *pdu_info = &pdu_buffer;

	key_list_t key_pdu_session_resource_setup_list_su_req = {0,};
	json_object *js_pdu_session_resource_setup_list_su_req = 
		search_json_object_ex(js_ngap_pdu, "/initiatingMessage/value/protocolIEs/{id:74, value}/", &key_pdu_session_resource_setup_list_su_req);

	if (js_pdu_session_resource_setup_list_su_req == NULL) {
		fprintf(stderr, "TODO %s() ue [%s]  fail, to get mandatory pdu_session_resource_setup_list_su_req!\n", __func__, UE_TAG(ue_ctx));
        goto UPSH_ERR;
	}

	pdu_info->pdu_num = 
		pdu_proc_fill_pdu_sess_setup_list(ue_ctx, js_pdu_session_resource_setup_list_su_req, pdu_info->pdu_sessions);

	ike_proc_pdu_request(ue_ctx, pdu_info);

	return 0;

UPSH_ERR:
	if (ue_ctx != NULL) {
		ue_ctx_unset(ue_ctx);
	}

	return -1;
}

int ue_pdu_setup_res_handle(ike_msg_t *ike_msg)
{
	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;
	ue_ctx_t *ue_ctx = ue_ctx_get_by_index(n3iwf_msg->ctx_info.cp_id, WORKER_CTX);

	if (ue_ctx == NULL) {
		fprintf(stderr, "TODO %s() called null ue_ctx!\n", __func__);
		return -1;
	}
	ue_ctx_stop_timer(ue_ctx);

	if (n3iwf_msg->res_code == N3_PDU_CREATE_SUCCESS) {
		/* send tcp-nas-pdu_accept */
		g_slist_foreach(ue_ctx->pdu_ctx_list, (GFunc)pdu_proc_sess_establish_accept, ue_ctx);
	} else {
		pdu_proc_flush_ctx(ue_ctx);
	}

	ngap_proc_pdu_session_resource_setup_response(ue_ctx, ike_msg);

	return 0;
}

int ue_ctx_release_handle(ngap_msg_t *ngap_msg, json_object *js_ngap_pdu)
{
    ue_ctx_t *ue_ctx = ue_ctx_get_by_index(ngap_msg->ngap_tag.id, WORKER_CTX);
    if (ue_ctx == NULL) {
		fprintf(stderr, "TODO %s() called null ue_ctx!\n", __func__);
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
		fprintf(stderr, "TODO %s() ue [%s]  fail, to get mandatory ngap_id or rel_cause!\n", __func__, UE_TAG(ue_ctx));
		goto UCRH_ERR;
	}

	ike_proc_ue_release(ue_ctx);

	return 0;

UCRH_ERR:
	return -1;
}

int ue_inform_res_handle(ike_msg_t *ike_msg)
{
	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;
	ue_ctx_t *ue_ctx = ue_ctx_get_by_index(n3iwf_msg->ctx_info.cp_id, WORKER_CTX);

	if (ue_ctx == NULL) {
		fprintf(stderr, "TODO %s() called null ue_ctx!\n", __func__);
		return -1;
	}
	ue_ctx_stop_timer(ue_ctx);

	switch (n3iwf_msg->res_code) {
		case N3_PDU_DELETE_SUCCESS:
		case N3_PDU_DELETE_FAILURE:
			ngap_proc_pdu_session_resource_release_response(ue_ctx, ike_msg);
			break;
		case N3_IPSEC_DELETE_UE_CTX:
			ngap_proc_ue_context_release_complete(ue_ctx);
			break;
	}

	return 0;
}
