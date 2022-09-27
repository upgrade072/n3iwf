#include <nwucp.h>

extern main_ctx_t *MAIN_CTX;
extern __thread worker_ctx_t *WORKER_CTX;

int create_ue_list(main_ctx_t *MAIN_CTX)
{
	config_setting_t *cfg_ue_ip_range = config_lookup(&MAIN_CTX->CFG, "n3iwf_info.ue_ip_range");
	const char *ip_range = config_setting_get_string(cfg_ue_ip_range);

	char ip_start[256] = {0,}, ip_end[256] = {0,};
	int ret = (ipaddr_range_scan(ip_range, ip_start, ip_end));
	if (ret <= 0) {
		fprintf(stderr, "%s fail to get ip range from=[%s]\n", __func__, ip_range);
		return (-1);
	} else {
		sprintf(MAIN_CTX->ue_info.ue_start_ip, "%s", ip_start);
	}

	int num_of_addr = ipaddr_range_calc(ip_start, ip_end);
	if (num_of_addr <= 0 || num_of_addr > MAX_NWUCP_UE_NUM) {
		fprintf(stderr, "%s fail cause num_of_addr=[%d] wrong\n", __func__, num_of_addr);
		return (-1);
	}
	MAIN_CTX->ue_info.ue_num = num_of_addr;
	MAIN_CTX->ue_info.netmask = ip_num_to_subnet(num_of_addr + 1 /* num of me + num of ue */);
	MAIN_CTX->ue_info.cur_index = 0; // for assign start pos
	MAIN_CTX->ue_info.ue_ctx = calloc(1, sizeof(ue_ctx_t) * MAIN_CTX->ue_info.ue_num);

	char *ip_list = strdup(ip_start);
	for (int i = 0; i < MAIN_CTX->ue_info.ue_num; i++) {
		ue_ctx_t *ue_ctx = &MAIN_CTX->ue_info.ue_ctx[i];
		ue_ctx->index = i;
		sprintf(ue_ctx->ip_addr, "%s", ip_list);
		ip_list = ipaddr_increaser(ip_list);
	}

	fprintf(stderr, "{dbg} %s() create ue from_%s ~ to_%s, num=(%d) netmask=(%d)\n", 
			__func__, ip_start, ip_end, MAIN_CTX->ue_info.ue_num, MAIN_CTX->ue_info.netmask);

	return (0);
}

ue_ctx_t *ue_ctx_assign(main_ctx_t *MAIN_CTX)
{
	for (int i = 0; i < MAIN_CTX->ue_info.ue_num; i++) {
		int index = (MAIN_CTX->ue_info.cur_index + 1 + i) % MAIN_CTX->ue_info.ue_num;
		ue_ctx_t *ue_ctx = &MAIN_CTX->ue_info.ue_ctx[index];
		if (ue_ctx->occupied == 0) {
			ue_ctx->occupied = 1;
			MAIN_CTX->ue_info.cur_index = index;
			fprintf(stderr, "{dbg} %s() assign ue ctx (index:%d, ip:%s)\n", __func__, index, ue_ctx->ip_addr);
			return ue_ctx;
		}
	}

	return NULL;
}

void ue_assign_by_ike_auth(ike_msg_t *ike_msg)
{
	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;
	ue_ctx_t *ue_ctx = ue_ctx_assign(MAIN_CTX);

	if (ue_ctx == NULL) {
		fprintf(stderr, "{TODO} %s() called null ue_ctx! reply to UP IKE Backoff noti\n", __func__);
		return;
	}
	memcpy(&ue_ctx->ike_tag, &ike_msg->ike_tag, sizeof(ike_tag_t));
	memcpy(&ue_ctx->ctx_info, &n3iwf_msg->ctx_info, sizeof(ctx_info_t));
	ue_ctx->ctx_info.cp_id = ue_ctx->index;

	worker_ctx_t *worker_ctx = worker_ctx_get_by_index(ue_ctx->index);

	event_base_once(worker_ctx->evbase_thrd, -1, EV_TIMEOUT, eap_proc_5g_start, ue_ctx, NULL);
}

ue_ctx_t *ue_ctx_get_by_index(int index, worker_ctx_t *worker_ctx)
{
	if (index >= MAIN_CTX->ue_info.ue_num) {
		fprintf(stderr, "{dbg} %s() called with invalid ue_index (%d), ue_num=(%d)!\n", 
				__func__, index, MAIN_CTX->ue_info.ue_num);
		return NULL;
	}
	if ((index % MAIN_CTX->IO_WORKERS.worker_num) != worker_ctx->thread_index) {
		fprintf(stderr, "{dbg} %s() called with invalid worker (%d), ue_index (%d) valid worker=(%d)!\n", 
				__func__, worker_ctx->thread_index, index, index % MAIN_CTX->IO_WORKERS.worker_num);
		return NULL;
	}

	ue_ctx_t *ue_ctx = &MAIN_CTX->ue_info.ue_ctx[index];
	if (ue_ctx->occupied == 0) {
		fprintf(stderr, "{dbg} %s() called with invalid ue_index (%d), it's unoccupied!\n", __func__, index);
		return NULL;
	}

	return ue_ctx;
}

void ue_ctx_transit_state(ue_ctx_t *ue_ctx, const char *new_state)
{
	fprintf(stderr, "%s() ue_ctx (cp_id:%d up_id:%d) state change [%s]=>[%s]\n", 
			__func__, ue_ctx->ctx_info.cp_id, ue_ctx->ctx_info.up_id, ue_ctx->state, new_state);

	ue_ctx->state = new_state;
}

void ue_ctx_stop_timer(ue_ctx_t *ue_ctx)
{
	if (ue_ctx->ev_timer != NULL) {
		event_del(ue_ctx->ev_timer);
		ue_ctx->ev_timer = NULL;
	}
}

void ue_ctx_release(int conn_fd, short events, void *data)
{
	ue_ctx_t *ue_ctx = (ue_ctx_t *)data;

	ue_ctx_unset(ue_ctx);
}

void ue_ctx_unset(ue_ctx_t *ue_ctx)
{
	ue_ctx_stop_timer(ue_ctx);

	fprintf(stderr, "{dbg} %s() release ue ctx at=[%s] (index:%d, ip:%s)\n", __func__, ue_ctx->state, ue_ctx->index, ue_ctx->ip_addr);

	ue_ctx->state = NULL;
	ue_ctx->occupied = 0;
}

int ue_compare_guami(an_param_t *an_param, json_object *js_guami)
{
	if (js_guami == NULL) {
		return -1;
	}

	an_guami_t *guami = &an_param->guami;

	char guami_str[128] = {0,};
	sprintf(guami_str, "%s%s%02X", guami->plmn_id_bcd, guami->amf_id_str, guami->amf_pointer);

	for (int i = 0; i < json_object_array_length(js_guami); i++) {
		json_object *js_elem = json_object_array_get_idx(js_guami, i);

		char compare_str[128] = {0,};
		sprintf(compare_str, "%s%s%s%s", 
				json_object_get_string(search_json_object(js_elem, "/gUAMI/pLMNIdentity")),
				json_object_get_string(search_json_object(js_elem, "/gUAMI/aMFRegionID")),
				json_object_get_string(search_json_object(js_elem, "/gUAMI/aMFSetID")),
				json_object_get_string(search_json_object(js_elem, "/gUAMI/aMFPointer")));

		if (!strcmp(guami_str, compare_str)) {
			fprintf(stderr, "{dbg} %s() find guami=[%s] in (%d)th elem of amf profile!\n", __func__, guami_str, i);
			return 1;
		}
	}

	return -1;
}

int ue_compare_nssai(const char *sstsd_str, json_object *js_slice_list)
{
	if (js_slice_list == NULL) {
		return -1;
	}

	for (int i = 0; i < json_object_array_length(js_slice_list); i++) {
		json_object *js_elem = json_object_array_get_idx(js_slice_list, i);

		char compare_str[128] = {0,};
		sprintf(compare_str, "%s%s",
				json_object_get_string(search_json_object(js_elem, "/s-NSSAI/sST")),
				json_object_get_string(search_json_object(js_elem, "/s-NSSAI/sD")));

		if (!strcmp(sstsd_str, compare_str)) {
			fprintf(stderr, "{dbg} %s() find sstsd=[%s] in (%d)th elem of amf profile (sliceSupportList)!\n", __func__, sstsd_str, i);
			return 1;
		}
	}

	return -1;
}

int ue_compare_plmn_nssai(an_param_t *an_param, json_object *js_plmn_id)
{
	if (js_plmn_id == NULL) {
		return -1;
	}

	an_plmn_id_t *plmn_id = &an_param->plmn_id;
	an_nssai_t   *nssai   = &an_param->nssai;

	for (int i = 0; i < json_object_array_length(js_plmn_id); i++) {
		json_object *js_elem = json_object_array_get_idx(js_plmn_id, i);

		const char *compare_plmn_id = json_object_get_string(search_json_object(js_elem, "/pLMNIdentity"));
		if (strcmp(plmn_id->plmn_id_bcd, compare_plmn_id)) {
			continue;
		}

		json_object *js_slice_list = search_json_object(js_elem, "/sliceSupportList");

		int find_num = 0;
		for (int k = 0; k < nssai->set_num; k++) {
			char sstsd_str[128] = {0,};
			sprintf(sstsd_str, "%02X%s", nssai->sst[k], nssai->sd_str[k]);
			if (ue_compare_nssai(sstsd_str, js_slice_list) > 0) {
				find_num ++;
			}
		}
		if (find_num != nssai->set_num) {
			continue;
		}

		return 1;
	}

	return -1;
}

int ue_check_an_param_with_amf(eap_relay_t *eap_5g, json_object *js_guami, json_object *js_plmn_id)
{
	an_param_t *an_param = &eap_5g->an_param;

	if ((an_param->guami.set && 
			ue_compare_guami(an_param, js_guami) < 0) ||
		(an_param->plmn_id.set && an_param->nssai.set_num && 
			ue_compare_plmn_nssai(an_param, js_plmn_id) < 0)) {
		return -1; /* not match */
	} else {
		return 1;  /* ok match */
	}
}

void ue_set_amf_by_an_param(ike_msg_t *ike_msg)
{
	eap_relay_t *eap_5g = &ike_msg->eap_5g;

	int find_amf = 0;
	for (int i = 0; i < link_node_length(&MAIN_CTX->amf_list); i++) {
		amf_ctx_t *amf_ctx = link_node_get_nth_data(&MAIN_CTX->amf_list, i);
		key_list_t key_list_guami = {0,};
		json_object *js_guami = search_json_object_ex(amf_ctx->js_amf_data,
				"/successfulOutcome/value/protocolIEs/{id:96, value}", &key_list_guami);
		key_list_t key_list_plmn_id = {0,};
		json_object *js_plmn_id = search_json_object_ex(amf_ctx->js_amf_data,
				"/successfulOutcome/value/protocolIEs/{id:80, value}", &key_list_plmn_id);

		if (ue_check_an_param_with_amf(eap_5g, js_guami, js_plmn_id) > 0) {
			find_amf = 1;
			sprintf(ike_msg->ike_tag.cp_amf_host, "%s", amf_ctx->hostname);
			fprintf(stderr, "{dbg} %s() find an_param match at amf=[%s] cp_id=(%d)\n", __func__, amf_ctx->hostname, ike_msg->n3iwf_msg.ctx_info.cp_id);
			break;
		}
	}
	if (!find_amf) {
		ike_msg->ike_tag.cp_amf_host[0] = '\0';
	}

	/* unset amf selection tag, adjust mtype by cp_id, relay to io_worker */
	eap_5g->an_param.set = 0;

	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;
	ike_msg->mtype = n3iwf_msg->ctx_info.cp_id % MAIN_CTX->IO_WORKERS.worker_num + 1;

	msgsnd(MAIN_CTX->QID_INFO.eap5g_nwucp_worker_qid, ike_msg, IKE_MSG_SIZE, IPC_NOWAIT);
}

int ue_check_ngap_id(ue_ctx_t *ue_ctx, json_object *js_ngap_pdu)
{
    int amf_ue_ngap_id = ngap_get_amf_ue_ngap_id(js_ngap_pdu);
    int ran_ue_ngap_id = ngap_get_ran_ue_ngap_id(js_ngap_pdu);

    if (amf_ue_ngap_id < 0 || ran_ue_ngap_id < 0) {
        fprintf(stderr, "{todo} %s() fail cause ngap_id not exist!\n", __func__);
		return -1;
	} else {
        ue_ctx->amf_tag.amf_ue_id = amf_ue_ngap_id;
        ue_ctx->amf_tag.ran_ue_id = ran_ue_ngap_id;
		return 0;
	}
}

void ue_regi_res_handle(ngap_msg_t *ngap_msg, json_object *js_ngap_pdu)
{
    ue_ctx_t *ue_ctx = ue_ctx_get_by_index(ngap_msg->ngap_tag.id, WORKER_CTX);
    if (ue_ctx == NULL) {
        fprintf(stderr, "{todo} %s() fail to get ue (index:%d)\n", __func__, ngap_msg->ngap_tag.id);
        goto URRH_ERR;
    } else {
        fprintf(stderr, "{dbg} %s() success to get ue (index:%d)\n", __func__, ue_ctx->index);
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
    } else {
        fprintf(stderr, "{dbg} %s() success to get ue (index:%d)\n", __func__, ue_ctx->index);
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
