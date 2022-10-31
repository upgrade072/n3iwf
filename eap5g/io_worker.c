#include <eapp.h>

extern main_ctx_t *MAIN_CTX;
static __thread worker_ctx_t *WORKER_CTX;

void handle_udp_request(int fd, short event, void *arg)
{   
	recv_buf_t *recv_buf = (recv_buf_t *)arg;
	if (recv_buf->size < (sizeof(n3iwf_msg_t) - 1)) {
		ERRLOG(LLE, FL, "%s() recv insufficient packet len!\n", __func__);
		goto HUR_END;
	}

	n3iwf_msg_t *n3iwf_msg = (n3iwf_msg_t *)recv_buf->buffer;
	if (N3IWF_MSG_SIZE(n3iwf_msg) != recv_buf->size) {
		ERRLOG(LLE, FL, "%s() recv invalid size!=(%ld) payload_len=(%d)\n", 
				__func__, N3IWF_MSG_SIZE(n3iwf_msg), htons(n3iwf_msg->payload_len));
		goto HUR_END;
	}

	handle_pkt_ntohs(n3iwf_msg);

	ike_msg_t msg = { .mtype = 1,}, *ike_msg = &msg;
	create_ike_tag(&ike_msg->ike_tag, &recv_buf->from_addr);
	memcpy(&ike_msg->n3iwf_msg, n3iwf_msg, sizeof(n3iwf_msg_t)); /* for save tag */

	ERRLOG(LLE, FL, "\n(%s:%d)=>(%s) [%s] [%s] (ue_id=%s up_id=%d cp_id=%d)\n",
			ike_msg->ike_tag.up_from_addr, ike_msg->ike_tag.up_from_port,
			WORKER_CTX->thread_name,
			n3_msg_code_str(n3iwf_msg->msg_code),
			n3_res_code_str(n3iwf_msg->res_code),
			n3iwf_msg->ctx_info.ue_id,
			n3iwf_msg->ctx_info.up_id,
			n3iwf_msg->ctx_info.cp_id);

	proc_udp_request(n3iwf_msg->msg_code, n3iwf_msg->res_code, n3iwf_msg, ike_msg);

	IKE_TRACE(n3iwf_msg, ike_msg, DIR_RECEIVE);

HUR_END:
	release_recv_buf(&MAIN_CTX->udp_buff, recv_buf);
}

void handle_ike_request(ike_msg_t *ike_msg)
{
	char pdu_buff[10240] = {0,};

	n3iwf_msg_t *n3iwf_msg = (n3iwf_msg_t *)pdu_buff;
	memcpy(n3iwf_msg, &ike_msg->n3iwf_msg, sizeof(n3iwf_msg_t));

	ERRLOG(LLE, FL, "\n(%s)=>(%s:%d) [%s] [%s] (ue_id=%s up_id=%d cp_id=%d)\n",
			WORKER_CTX->thread_name,
			ike_msg->ike_tag.up_from_addr, ike_msg->ike_tag.up_from_port,
			n3_msg_code_str(n3iwf_msg->msg_code),
			n3_res_code_str(n3iwf_msg->res_code),
			n3iwf_msg->ctx_info.ue_id,
			n3iwf_msg->ctx_info.up_id,
			n3iwf_msg->ctx_info.cp_id);

	proc_msg_request(n3iwf_msg->msg_code, n3iwf_msg->res_code, ike_msg, n3iwf_msg);

	IKE_TRACE(n3iwf_msg, ike_msg, DIR_SEND);

	handle_pkg_htons(n3iwf_msg);

	socklen_t tl = sizeof(struct sockaddr_in);
	struct sockaddr_in to_addr;
	memset(&to_addr, 0x00, sizeof(to_addr));
	to_addr.sin_family = AF_INET;
	to_addr.sin_addr.s_addr = inet_addr(ike_msg->ike_tag.up_from_addr);
	to_addr.sin_port = htons(MAIN_CTX->ike_listen_port);

	sendfromto(WORKER_CTX->udp_sock, n3iwf_msg, N3IWF_MSG_SIZE(n3iwf_msg), 0, NULL, 0, (struct sockaddr *)&to_addr, tl);
}

/* all worker race condition */
void msg_rcv_from_nwucp(int fd, short event, void *arg)
{   
	ike_msg_t msg = {0,}, *ike_msg = &msg;

	while (msgrcv(MAIN_CTX->QID_INFO.nwucp_eap5g_qid, ike_msg, sizeof(ike_msg_t), 0, IPC_NOWAIT) > 0) {
		handle_ike_request(ike_msg);
	}
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

	struct timeval u_sec = {0, 1};
	struct event *ev_msg_rcv_from_nwucp = event_new(WORKER_CTX->evbase_thrd, -1, EV_PERSIST, msg_rcv_from_nwucp, NULL);
    event_add(ev_msg_rcv_from_nwucp, &u_sec);

    event_base_loop(WORKER_CTX->evbase_thrd, EVLOOP_NO_EXIT_ON_EMPTY);
        
    return NULL;
}
