#include <eapp.h>

extern main_ctx_t *MAIN_CTX;
static __thread worker_ctx_t *WORKER_CTX;

typedef struct nwucp_ike_msg_t {
	long		mtype;		/* 1 or worker qid (1,2,3,4...) */
	char		ue_id[128];
	n3iwf_msg_t n3iwf_msg;	/* warning! not include whole payload */
	eap_relay_t eap_5g;	
} nwucp_ike_msg_t;

void handle_udp_ntohs(n3iwf_msg_t *n3iwf_msg)
{
	n3iwf_msg->ctx_info.up_id = ntohs(n3iwf_msg->ctx_info.up_id);
	n3iwf_msg->ctx_info.cp_id = ntohs(n3iwf_msg->ctx_info.cp_id);
	n3iwf_msg->payload_len = ntohs(n3iwf_msg->payload_len);
}

void handle_udp_request(int fd, short event, void *arg)
{   
	recv_buf_t *recv_buf = (recv_buf_t *)arg;

	char from_addr[INET_ADDRSTRLEN] = {0,};
	inet_ntop(AF_INET, &recv_buf->from_addr.sin_addr, from_addr, INET_ADDRSTRLEN);
	int  from_port = ntohs(recv_buf->from_addr.sin_port);

	fprintf(stderr, "{dbg} [%s] recv! from=(%s:%d)\n", WORKER_CTX->thread_name, from_addr, from_port);

	if (recv_buf->size < sizeof(n3iwf_msg_t)) {
		fprintf(stderr, "%s() recv invalid packet len!\n", __func__);
		goto HUR_END;
	}

	n3iwf_msg_t *n3iwf_msg = (n3iwf_msg_t *)recv_buf->buffer;
	handle_udp_ntohs(n3iwf_msg);

	if ((sizeof(n3iwf_msg_t) - 1 + n3iwf_msg->payload_len) != recv_buf->size) {
		fprintf(stderr, "%s() recv invalid payload len!\n", __func__);
		goto HUR_END;
	}

	nwucp_ike_msg_t msg = { .mtype = 0, .ue_id = {0,} }, *nwucp_ike_msg = &msg;
	memcpy(&nwucp_ike_msg->n3iwf_msg, n3iwf_msg, sizeof(n3iwf_msg_t)); /* for save tag */
	eap_relay_t *eap_5g = &nwucp_ike_msg->eap_5g;

	switch (n3iwf_msg->msg_code) {
		case N3_IKE_AUTH_INIT:
			sprintf(nwucp_ike_msg->ue_id, "%.127s", n3iwf_msg->payload);
			break;
		case N3_IKE_AUTH_REQ:
			memset(eap_5g, 0x00, sizeof(eap_relay_t));
			decap_eap_res(eap_5g, (unsigned char *)n3iwf_msg->payload, n3iwf_msg->payload_len);
			break;
	}

HUR_END:
	release_recv_buf(&MAIN_CTX->udp_buff, recv_buf);
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

    event_base_loop(WORKER_CTX->evbase_thrd, EVLOOP_NO_EXIT_ON_EMPTY);
        
    return NULL;
}
