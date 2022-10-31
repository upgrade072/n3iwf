#include <nwucp.h>

extern main_ctx_t *MAIN_CTX;
__thread worker_ctx_t *WORKER_CTX;

void handle_ngap_log(const char *prefix, ngap_msg_t *ngap_msg, event_caller_t caller)
{
	sctp_tag_t *sctp_tag = &ngap_msg->sctp_tag;

	fprintf(stderr, "\nsctp recv [%s] from [host:%s assoc:%d stream:%d ppid:%d] with (%s)\n",
			prefix,
			sctp_tag->hostname, sctp_tag->assoc_id, sctp_tag->stream_id, sctp_tag->ppid, 
			caller == EC_MAIN ? "main" : WORKER_CTX->thread_name);
}

void handle_ngap_msg(ngap_msg_t *ngap_msg, event_caller_t caller)
{
	/* caution! must put this object */
	json_object *js_ngap_pdu = json_tokener_parse(ngap_msg->msg_body);

	key_list_t *key_list = &ngap_msg->ngap_tag.key_list;
	memset(key_list, 0x00, sizeof(key_list_t));

	int proc_code = json_object_get_int(search_json_object_ex(js_ngap_pdu, "/*/procedureCode", key_list));
	int success = !strcmp(key_list->key_val[0], "successfulOutcome") ? true : false;

	handle_ngap_log(NGAP_PROC_C_STR(proc_code), ngap_msg, caller);

	/* save sctp_tag to ue_ctx & TRACE */
	ue_save_sctp_tag(ngap_msg, js_ngap_pdu, caller);

	/* check overload status */
	if (NWUCP_OVLD_CHK_NGAP(proc_code) < 0) {
		goto HNM_DISCARD;
	}

	switch (proc_code) 
	{
		/* AMF MESSAGE (HANDLE BY MAIN) */
		case NGAP_AMFStatusIndication:
			amf_status_ind_handle(ngap_msg, js_ngap_pdu);
			break;
		case NGAP_NGSetup:
			amf_regi_res_handle(ngap_msg, success, js_ngap_pdu);
			break;
		/* UE MESSAGE (HANDLE BY WORKER) */
		case NGAP_DownlinkNASTransport:
			nas_relay_to_ue(ngap_msg, js_ngap_pdu);
			break;
		case NGAP_InitialContextSetup:
			ue_regi_res_handle(ngap_msg, js_ngap_pdu);
			break;
		case NGAP_PDUSessionResourceRelease:
			ue_pdu_release_req_handle(ngap_msg, js_ngap_pdu);
			break;
		case NGAP_PDUSessionResourceSetup:
			ue_pdu_setup_req_handle(ngap_msg, js_ngap_pdu);
			break;
		case NGAP_UEContextRelease:
			ue_ctx_release_handle(ngap_msg, js_ngap_pdu);
			break;
		default:
			/* we can't handle, just discard */
			fprintf(stderr, "%s() recv [Unknown NGAP Message=(%d)!] with (%s)\n%s\n", 
					__func__, proc_code, caller == EC_MAIN ? "main" : WORKER_CTX->thread_name, JS_PRINT_PRETTY(js_ngap_pdu));
			break;
	}

HNM_DISCARD:
	if (js_ngap_pdu != NULL) {
		json_object_put(js_ngap_pdu);
	}
}

void handle_ike_log(const char *prefix, ike_msg_t *ike_msg, event_caller_t caller)
{
	fprintf(stderr, "\nike recv [%s] from [ue_ip:%s ue_port:%d up_ip:%s up_port:%d rel_amf:%s] with (%s)\n",
			prefix,
			ike_msg->ike_tag.ue_from_addr,
			ike_msg->ike_tag.ue_from_port,
			ike_msg->ike_tag.up_from_addr,
			ike_msg->ike_tag.up_from_port,
			ike_msg->ike_tag.cp_amf_host,
			caller == EC_MAIN ? "main" : WORKER_CTX->thread_name);
}

void handle_ike_msg(ike_msg_t *ike_msg, event_caller_t caller)
{
	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;
	eap_relay_t *eap_5g = &ike_msg->eap_5g;

	/* check overload status */
	if (NWUCP_OVLD_CHK_EAP(n3iwf_msg) < 0) {
		return;
	}

	switch (n3iwf_msg->msg_code)
	{
		case N3_IKE_AUTH_REQ:
			handle_ike_log("N3_IKE_AUTH_REQ", ike_msg, caller);
			if (n3iwf_msg->res_code == N3_EAP_INIT) {
				return ue_assign_by_ike_auth(ike_msg);
			}
			if (eap_5g->an_param.set == RS_AN_PARAM_SET) {
				/* main (select amf) --msgsnd--> worker */
				return ue_set_amf_by_an_param(ike_msg);
			} else {
				return nas_relay_to_amf(ike_msg);
			}
		case N3_IKE_IPSEC_NOTI:
			handle_ike_log("N3_IKE_IPSEC_NOTI", ike_msg, caller);
			if (n3iwf_msg->res_code == N3_IPSEC_UE_DISCONNECT) {
				return ue_context_release(ike_msg);
			} else {
				return nas_regi_to_amf(ike_msg);
			}
		case N3_IKE_INFORM_RES:
			handle_ike_log("N3_IKE_INFORM_RES", ike_msg, caller);
			return ue_inform_res_handle(ike_msg);
		case N3_CREATE_CHILD_SA_RES:
			handle_ike_log("N3_CREATE_CHILD_SA_RES", ike_msg, caller);
			return ue_pdu_setup_res_handle(ike_msg);
		default:
			/* we can't handle, just discard */
			fprintf(stderr, "%s() recv [Unknown IKE Message=(%d)!] with (%s)\n", 
					__func__, n3iwf_msg->msg_code, caller == EC_MAIN ? "main" : WORKER_CTX->thread_name);
			return;
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

		//fprintf(stderr, "%s() [%s:%s] recv from msgq\n", __func__, caller, strcmp(caller, "main") ? WORKER_CTX->thread_name : "");

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

		//fprintf(stderr, "%s() [%s:%s] recv from msgq\n", __func__, caller, strcmp(caller, "main") ? WORKER_CTX->thread_name : "");

		handle_ike_msg(ike_msg, ev_caller);
	}
}

worker_ctx_t *worker_ctx_get_by_index(int index)
{
	return &MAIN_CTX->IO_WORKERS.workers[index % MAIN_CTX->IO_WORKERS.worker_num];
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
