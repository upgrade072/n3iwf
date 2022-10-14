#include <eapp.h>

extern main_ctx_t *MAIN_CTX;

void proc_eap_init(n3iwf_msg_t *n3iwf_msg, ike_msg_t *ike_msg)
{
	n3_eap_init_t *eap_init	= (n3_eap_init_t *)n3iwf_msg->payload;

	sprintf(ike_msg->ike_tag.ue_from_addr, "%s", eap_init->ue_from_addr);
	ike_msg->ike_tag.ue_from_port = ntohs(eap_init->ue_from_port);

	fprintf(stderr, "{DBG} %s() recv from ue (%s:%d)\n", __func__, ike_msg->ike_tag.ue_from_addr, ike_msg->ike_tag.ue_from_port);

	msgsnd(MAIN_CTX->QID_INFO.eap5g_nwucp_qid, ike_msg, IKE_MSG_SIZE, IPC_NOWAIT);
}

void proc_eap_request(ike_msg_t *ike_msg, n3iwf_msg_t *n3iwf_msg)
{
	eap_relay_t *eap_5g = &ike_msg->eap_5g;

	n3iwf_msg->payload_len = encap_eap_req(eap_5g, (unsigned char *)n3iwf_msg->payload, 10240 - sizeof(n3iwf_msg_t));
}

void proc_eap_response(n3iwf_msg_t *n3iwf_msg, ike_msg_t *ike_msg)
{
	eap_relay_t *eap_5g	= &ike_msg->eap_5g;
	an_param_t *an_param = &eap_5g->an_param;

	memset(eap_5g, 0x00, sizeof(eap_relay_t));
	decap_eap_res(eap_5g, (unsigned char *)n3iwf_msg->payload, n3iwf_msg->payload_len);

	/* if an_param exist, send to main, or send to cp_id related worker  */
	if (an_param->set == 0) {
		ike_msg->mtype = n3iwf_msg->ctx_info.cp_id % MAIN_CTX->DISTR_INFO.worker_num + 1;
	}
	msgsnd(an_param->set ? MAIN_CTX->QID_INFO.eap5g_nwucp_qid : MAIN_CTX->DISTR_INFO.worker_distr_qid, 
			ike_msg, IKE_MSG_SIZE, IPC_NOWAIT);
}

void proc_ipsec_noti(n3iwf_msg_t *n3iwf_msg, ike_msg_t *ike_msg)
{
	ike_msg->mtype = n3iwf_msg->ctx_info.cp_id % MAIN_CTX->DISTR_INFO.worker_num + 1;
	msgsnd(MAIN_CTX->DISTR_INFO.worker_distr_qid, ike_msg, IKE_MSG_SIZE, IPC_NOWAIT);
}

void proc_inform_response(n3iwf_msg_t *n3iwf_msg, ike_msg_t *ike_msg)
{
	n3_pdu_info_t *pdu_info = &ike_msg->ike_data.pdu_info;
	ike_msg->ike_choice		= choice_pdu_info;

	if (n3iwf_msg->res_code == N3_PDU_DELETE_SUCCESS) {
		memcpy(pdu_info, n3iwf_msg->payload, n3iwf_msg->payload_len);

		for (int i = 0; i < pdu_info->pdu_num; i++) {
			n3_pdu_sess_t *pdu_sess = &pdu_info->pdu_sessions[i];
			pdu_sess->pdu_sess_ambr_dl = ntohl(pdu_sess->pdu_sess_ambr_dl);
			pdu_sess->pdu_sess_ambr_ul = ntohl(pdu_sess->pdu_sess_ambr_ul);
			fprintf(stderr, "{DBG} %s() recv (%d) (%d) (%d) (%s) (%s) (%d)\n", __func__,
					pdu_sess->session_id,
					pdu_sess->pdu_sess_ambr_dl,
					pdu_sess->pdu_sess_ambr_ul,
					pdu_sess->gtp_te_addr,
					pdu_sess->gtp_te_id,
					pdu_sess->address_family);
		}
	}

	ike_msg->mtype = n3iwf_msg->ctx_info.cp_id % MAIN_CTX->DISTR_INFO.worker_num + 1;
	msgsnd(MAIN_CTX->DISTR_INFO.worker_distr_qid, ike_msg, IKE_MSG_SIZE, IPC_NOWAIT);
}

void proc_eap_result(ike_msg_t *ike_msg, n3iwf_msg_t *n3iwf_msg)
{
	n3_eap_result_t *eap_result = (n3_eap_result_t *)n3iwf_msg->payload;
	memcpy(eap_result, &ike_msg->ike_data.eap_result, sizeof(n3_eap_result_t));
	eap_result->tcp_server_port = htons(eap_result->tcp_server_port);

	eap_relay_t *eap_5g = &ike_msg->eap_5g;
	encap_eap_result(eap_5g, (unsigned char *)eap_result->eap_payload, 4);
	n3iwf_msg->payload_len = sizeof(n3_eap_result_t);
}

void proc_inform_request(ike_msg_t *ike_msg, n3iwf_msg_t *n3iwf_msg)
{
	n3_pdu_info_t *pdu_info = (n3_pdu_info_t *)n3iwf_msg->payload;
	memcpy(pdu_info, &ike_msg->ike_data.pdu_info, N3_PDU_INFO_SIZE(pdu_info));

	for (int i = 0; i < pdu_info->pdu_num; i++) {
		n3_pdu_sess_t *pdu_sess = &pdu_info->pdu_sessions[i];
		fprintf(stderr, "{DBG} %s() send (%d) (%d) (%d) (%s) (%s) (%d)\n", __func__,
				pdu_sess->session_id,
				pdu_sess->pdu_sess_ambr_dl,
				pdu_sess->pdu_sess_ambr_ul,
				pdu_sess->gtp_te_addr,
				pdu_sess->gtp_te_id,
				pdu_sess->address_family);
		pdu_sess->pdu_sess_ambr_dl = htonl(pdu_sess->pdu_sess_ambr_dl);
		pdu_sess->pdu_sess_ambr_ul = htonl(pdu_sess->pdu_sess_ambr_ul);
	}

	n3iwf_msg->payload_len = N3_PDU_INFO_SIZE(pdu_info);
}

void proc_pdu_request(ike_msg_t *ike_msg, n3iwf_msg_t *n3iwf_msg)
{
	n3_pdu_info_t *pdu_info = (n3_pdu_info_t *)n3iwf_msg->payload;
	memcpy(pdu_info, &ike_msg->ike_data.pdu_info, N3_PDU_INFO_SIZE(pdu_info));

	for (int i = 0; i < pdu_info->pdu_num; i++) {
		n3_pdu_sess_t *pdu_sess = &pdu_info->pdu_sessions[i];
		fprintf(stderr, "{DBG} %s() send (%d) (%d) (%d) (%s) (%s) (%d)\n", __func__,
				pdu_sess->session_id,
				pdu_sess->pdu_sess_ambr_dl,
				pdu_sess->pdu_sess_ambr_ul,
				pdu_sess->gtp_te_addr,
				pdu_sess->gtp_te_id,
				pdu_sess->address_family);
		pdu_sess->pdu_sess_ambr_dl = htonl(pdu_sess->pdu_sess_ambr_dl);
		pdu_sess->pdu_sess_ambr_ul = htonl(pdu_sess->pdu_sess_ambr_ul);
	}

	n3iwf_msg->payload_len = N3_PDU_INFO_SIZE(pdu_info);
}

void proc_pdu_response(n3iwf_msg_t *n3iwf_msg, ike_msg_t *ike_msg)
{
	n3_pdu_info_t *pdu_info = &ike_msg->ike_data.pdu_info;
	ike_msg->ike_choice		= choice_pdu_info;

	if (n3iwf_msg->res_code == N3_PDU_CREATE_SUCCESS) {
		memcpy(pdu_info, n3iwf_msg->payload, n3iwf_msg->payload_len);

		for (int i = 0; i < pdu_info->pdu_num; i++) {
			n3_pdu_sess_t *pdu_sess = &pdu_info->pdu_sessions[i];
			pdu_sess->pdu_sess_ambr_dl = ntohl(pdu_sess->pdu_sess_ambr_dl);
			pdu_sess->pdu_sess_ambr_ul = ntohl(pdu_sess->pdu_sess_ambr_ul);
			fprintf(stderr, "{DBG} %s() recv (%d) (%d) (%d) (%s) (%s) (%d)\n", __func__,
					pdu_sess->session_id,
					pdu_sess->pdu_sess_ambr_dl,
					pdu_sess->pdu_sess_ambr_ul,
					pdu_sess->gtp_te_addr,
					pdu_sess->gtp_te_id,
					pdu_sess->address_family);
		}
	}

	ike_msg->mtype = n3iwf_msg->ctx_info.cp_id % MAIN_CTX->DISTR_INFO.worker_num + 1;
	msgsnd(MAIN_CTX->DISTR_INFO.worker_distr_qid, ike_msg, IKE_MSG_SIZE, IPC_NOWAIT);
}

void proc_udp_request(int msg_code, int res_code, n3iwf_msg_t *n3iwf_msg, ike_msg_t *ike_msg)
{
	switch (msg_code) {
		case N3_IKE_AUTH_REQ:
			if (res_code == N3_EAP_INIT) {
				proc_eap_init(n3iwf_msg, ike_msg);
			} else {
				proc_eap_response(n3iwf_msg, ike_msg);
			}
			break;
		case N3_IKE_IPSEC_NOTI:
			proc_ipsec_noti(n3iwf_msg, ike_msg);
			break;
		case N3_IKE_INFORM_RES:
			proc_inform_response(n3iwf_msg, ike_msg);
			break;
		case N3_CREATE_CHILD_SA_RES:
			proc_pdu_response(n3iwf_msg, ike_msg);
			break;
	}
}

void proc_msg_request(int msg_code, int res_code, ike_msg_t *ike_msg, n3iwf_msg_t *n3iwf_msg)
{
	switch (msg_code) {
		case N3_IKE_AUTH_RES:
			if (res_code == N3_EAP_REQUEST) {
				return proc_eap_request(ike_msg, n3iwf_msg);
			} else {
				return proc_eap_result(ike_msg, n3iwf_msg);
			}
		case N3_IKE_INFORM_REQ:
			return proc_inform_request(ike_msg, n3iwf_msg);
		case N3_CREATE_CHILD_SA_REQ:
			return proc_pdu_request(ike_msg, n3iwf_msg);
	}
}
