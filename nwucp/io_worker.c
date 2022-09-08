#include <nwucp.h>

extern main_ctx_t *MAIN_CTX;
static __thread worker_ctx_t *WORKER_CTX;

void handle_ngap_msg(ngap_msg_t *ngap_msg, event_caller_t caller)
{
	sctp_tag_t *sctp_tag = &ngap_msg->sctp_tag;
	fprintf(stderr, "{dbg} from sctp [h:%s a:%d s:%d p:%d]\n", 
			sctp_tag->hostname, sctp_tag->assoc_id, sctp_tag->stream_id, sctp_tag->ppid);

	/* caution! must put this object */
	json_object *js_ngap_pdu = json_tokener_parse(ngap_msg->msg_body);

	key_list_t *key_list = &ngap_msg->ngap_tag.key_list;
	memset(key_list, 0x00, sizeof(key_list_t));

	int proc_code = json_object_get_int(search_json_object_ex(js_ngap_pdu, "/*/procedureCode", key_list));
	int success = !strcmp(key_list->key_val[0], "successfulOutcome") ? true : false;

	switch (proc_code) 
	{
		case NGSetupResponse:
			return amf_regi_res_handle(sctp_tag, success, js_ngap_pdu);
		default:
			/* we can't handle, just discard */
			if (js_ngap_pdu != NULL) {
				json_object_put(js_ngap_pdu);
			}
			break;
	}
}

void handle_ike_msg(ike_msg_t *ike_msg, event_caller_t caller)
{
	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;

	switch (n3iwf_msg->msg_code)
	{
		case N3_IKE_AUTH_INIT:
			return ike_auth_assign_ue(ike_msg);
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

		fprintf(stderr, "{dbg} %s() [%s%s] recv msg!\n", __func__, caller, strcmp(caller, "main") ? WORKER_CTX->thread_name : "");

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

		fprintf(stderr, "{dbg} %s() [%s%s] recv msg!\n", __func__, caller, strcmp(caller, "main") ? WORKER_CTX->thread_name : "");

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
