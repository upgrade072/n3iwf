#include <eapp.h>

extern main_ctx_t *MAIN_CTX;
static __thread worker_ctx_t *WORKER_CTX;

void handle_udp_ntohs(n3iwf_msg_t *n3iwf_msg)
{
	n3iwf_msg->ctx_info.up_id = ntohs(n3iwf_msg->ctx_info.up_id);
	n3iwf_msg->ctx_info.cp_id = ntohs(n3iwf_msg->ctx_info.cp_id);
	n3iwf_msg->payload_len = ntohs(n3iwf_msg->payload_len);
}

void handle_udp_htons(n3iwf_msg_t *n3iwf_msg)
{
	n3iwf_msg->ctx_info.up_id = htons(n3iwf_msg->ctx_info.up_id);
	n3iwf_msg->ctx_info.cp_id = htons(n3iwf_msg->ctx_info.cp_id);
	n3iwf_msg->payload_len = htons(n3iwf_msg->payload_len);
}

void handle_udp_request(int fd, short event, void *arg)
{   
	recv_buf_t *recv_buf = (recv_buf_t *)arg;

	char from_addr[INET_ADDRSTRLEN] = {0,};
	inet_ntop(AF_INET, &recv_buf->from_addr.sin_addr, from_addr, INET_ADDRSTRLEN);
	int  from_port = ntohs(recv_buf->from_addr.sin_port);

	fprintf(stderr, "{dbg} [%s] %s()! from=(%s:%d)\n", WORKER_CTX->thread_name, __func__, from_addr, from_port);

	if (recv_buf->size < sizeof(n3iwf_msg_t)) {
		fprintf(stderr, "%s() recv invalid packet len!\n", __func__);
		goto HUR_END;
	}

	n3iwf_msg_t *n3iwf_msg = (n3iwf_msg_t *)recv_buf->buffer;

	if (N3IWF_MSG_SIZE(n3iwf_msg) != recv_buf->size) {
		fprintf(stderr, "%s() recv invalid payload len!\n", __func__);
		goto HUR_END;
	}

	handle_udp_ntohs(n3iwf_msg);

	/* create relay message */
	ike_msg_t msg = { .mtype = 1,}, *ike_msg = &msg;
	sprintf(ike_msg->ike_tag.from_addr, "%s", from_addr);
	memcpy(&ike_msg->n3iwf_msg, n3iwf_msg, sizeof(n3iwf_msg_t)); /* for save tag */
	eap_relay_t *eap_5g = &ike_msg->eap_5g;

	fprintf(stderr, "{dbg} up_id=(%d) cp_id=(%d) payload_len=(%d)\n", 
			n3iwf_msg->ctx_info.up_id,
			n3iwf_msg->ctx_info.cp_id,
			n3iwf_msg->payload_len);

	int res = 0;
	switch (n3iwf_msg->msg_code) {
		case N3_IKE_AUTH_INIT:
			sprintf(ike_msg->ike_tag.ue_id, "%.127s", n3iwf_msg->payload);
			res = msgsnd(MAIN_CTX->QID_INFO.eap5g_nwucp_qid, ike_msg, IKE_MSG_SIZE, IPC_NOWAIT);
			break;
		case N3_IKE_AUTH_REQ:
			memset(eap_5g, 0x00, sizeof(eap_relay_t));
			decap_eap_res(eap_5g, (unsigned char *)n3iwf_msg->payload, n3iwf_msg->payload_len);
			/* if an_param->set --> send to main (for AMF selection) */
			break;
	}
	fprintf(stderr, "{dbg} %s send res=(%d) if err (%d:%s)\n", __func__, res, errno, strerror(errno));

HUR_END:
	release_recv_buf(&MAIN_CTX->udp_buff, recv_buf);
}

void handle_ike_request(ike_msg_t *ike_msg)
{
	const char *from_addr = ike_msg->ike_tag.from_addr;
	eap_relay_t *eap_5g = &ike_msg->eap_5g;

	char pdu_buff[10240] = {0,};
	n3iwf_msg_t *n3iwf_msg = (n3iwf_msg_t *)pdu_buff;
	memcpy(n3iwf_msg, &ike_msg->n3iwf_msg, sizeof(n3iwf_msg_t));
	n3iwf_msg->payload_len = encap_eap_req(eap_5g, (unsigned char *)n3iwf_msg->payload, 10240 - sizeof(n3iwf_msg_t));

	handle_udp_htons(n3iwf_msg);

	socklen_t tl = sizeof(struct sockaddr_in);
	struct sockaddr_in to_addr;
	memset(&to_addr, 0x00, sizeof(to_addr));
	to_addr.sin_family = AF_INET;
	to_addr.sin_addr.s_addr = inet_addr(from_addr);
	to_addr.sin_port = htons(MAIN_CTX->ike_listen_port);

	int res = sendfromto(WORKER_CTX->udp_sock, n3iwf_msg, N3IWF_MSG_SIZE(n3iwf_msg), 0, NULL, 0, (struct sockaddr *)&to_addr, tl);

	fprintf(stderr, "sendres=(%d) (%d:%s)\n", res, errno, strerror(errno));
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
