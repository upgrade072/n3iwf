#include <eapp.h>

extern main_ctx_t *MAIN_CTX;

void proc_eap_init(n3iwf_msg_t *n3iwf_msg, ike_msg_t *ike_msg)
{
	n3_eap_init_t *eap_init	= &ike_msg->ike_data.eap_init; 
	eap_init->ue_from_port = htons(eap_init->ue_from_port);
	memcpy(eap_init, n3iwf_msg->payload, sizeof(n3_eap_init_t));
	msgsnd(MAIN_CTX->QID_INFO.eap5g_nwucp_qid, ike_msg, IKE_MSG_SIZE, IPC_NOWAIT);
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

void proc_eap_request(ike_msg_t *ike_msg, n3iwf_msg_t *n3iwf_msg)
{
	eap_relay_t *eap_5g = &ike_msg->eap_5g;

	n3iwf_msg->payload_len = encap_eap_req(eap_5g, (unsigned char *)n3iwf_msg->payload, 10240 - sizeof(n3iwf_msg_t));
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
	}
}
