#include "sctpc.h"

extern main_ctx_t *MAIN_CTX;
static __thread worker_ctx_t *WORKER_CTX;

static struct sctp_event_subscribe SCTP_NOTIF_EVENT = {
	.sctp_data_io_event = 1,
	.sctp_association_event = 1,
	.sctp_address_event = 1,
	.sctp_send_failure_event = 1,
	.sctp_peer_error_event = 1,
	.sctp_shutdown_event = 1,
	.sctp_partial_delivery_event = 1,
	.sctp_adaptation_layer_event = 1,
	.sctp_authentication_event = 1,
	.sctp_sender_dry_event = 0, // if send data dry out 
};

static struct sctp_initmsg SCTP_INIT_MESSAGE = {
	.sinit_num_ostreams = 5,
	.sinit_max_instreams = 5,
	.sinit_max_attempts = 4,
	/* .sinit_max_init_timeo = 0 */
};

static struct sctp_paddrparams SCTP_PADDR_PARAMS = {
	.spp_flags = SPP_HB_ENABLE,
	.spp_hbinterval = 1000,
	.spp_pathmaxrxt = 5,
	/* TODO : check disable status */
};

void sock_write(int conn_fd, short events, void *data)
{
	conn_status_t *conn = (conn_status_t *)data;

	if (!TAILQ_EMPTY(&conn->queue_head)) {
		tailq_entry *item_now = NULL;
		while ((item_now = TAILQ_FIRST(&conn->queue_head))) {
			recv_buf_t *buffer = item_now->send_item;
			sctp_msg_t *sctp_msg = (sctp_msg_t *)buffer->buffer;

			struct sctp_sndrcvinfo info = {
				.sinfo_stream = sctp_msg->tag.stream_id,
				.sinfo_ssn = 0,
				.sinfo_flags = 0,
				.sinfo_ppid = htonl(sctp_msg->tag.ppid),
				.sinfo_context = 0,
				.sinfo_timetolive = 0,
				.sinfo_tsn = 0,
				.sinfo_cumtsn = 0, 
				.sinfo_assoc_id = conn->assoc_id };
			int send_bytes = sctp_send(conn->conns_fd, sctp_msg->msg_body, sctp_msg->msg_size, &info, MSG_DONTWAIT);

			if (send_bytes > 0) {
				buffer->occupied = 0;
			} else {
				if (errno != EAGAIN) {
					fprintf(stderr, "{dbg} %s() error occured (%d:%s)\n", __func__, errno, strerror(errno));
				}
				return;
			}
			TAILQ_REMOVE(&conn->queue_head, item_now, items);
			free(item_now);
		}
	}
}

void handle_sock_send(int conn_fd, short events, void *data)
{
	recv_buf_t *buffer = (recv_buf_t *)data;
	sctp_msg_t *sctp_msg = (sctp_msg_t *)buffer->buffer;

	sctp_client_t *client = link_node_get_data_by_key(&WORKER_CTX->conn_list, sctp_msg->tag.hostname);
	if (!client->enable) {
		goto HSS_FAIL;
	}

	conn_status_t *target_conn = search_connection(client, sctp_msg);
	if (target_conn == NULL) {
		goto HSS_FAIL;
	}

	tailq_entry *item = malloc(sizeof(tailq_entry));
	item->send_item = buffer;
	TAILQ_INSERT_TAIL(&target_conn->queue_head, item, items);

	event_base_once(WORKER_CTX->evbase_thrd, -1, EV_TIMEOUT, sock_write, target_conn, NULL);

	return;

HSS_FAIL:
	buffer->occupied = 0;	
}

void relay_msg_to_ppid(sctp_msg_t *send_msg)
{
	for (int i = 0; i < MAIN_CTX->QID_INFO.recv_relay_num; i++) {
		ppid_pqid_t *pqid = MAIN_CTX->QID_INFO.recv_relay;
		if (pqid->sctp_ppid == send_msg->tag.ppid) {
			msgsnd(pqid->proc_pqid, send_msg, SCTP_MSG_SIZE(send_msg), IPC_NOWAIT);
			fprintf(stderr, "{dbg} %s send to msg to proc\n", __func__);
			return;
		}
	}
}

void handle_sock_recv(int conn_fd, short events, void *data)
{
	(void)events;

	conn_status_t *conn_status = (conn_status_t *)data;

	struct sctp_sndrcvinfo sinfo = {0,};
	sctp_msg_t msg = {0,}; char *buffer = msg.msg_body;
	struct sockaddr from = {0,};
	socklen_t from_len = sizeof(from);
	int recv_bytes = 0; int msg_flags = 0;

RECV_MORE:
	msg_flags = 0 | MSG_DONTWAIT;
	recv_bytes = msg.msg_size = sctp_recvmsg(conn_fd, buffer, MAX_SCTP_BUFF_SIZE, &from, &from_len, &sinfo, &msg_flags);

	if (msg_flags & MSG_NOTIFICATION) {

		/* Additional Info */
		const char *sctp_state = NULL;
		conn_status->assoc_state = get_assoc_state(conn_status->conns_fd, conn_status->assoc_id, &sctp_state);
		fprintf(stderr, "%s() update assoc(%d) state=[%d:%s]\n", __func__, conn_status->assoc_id, conn_status->assoc_state, sctp_state);

		/* Maybe restart conn */
		const char *event_str = NULL, *event_state = NULL;
		int ret = handle_sctp_notification((union sctp_notification *)buffer, recv_bytes, &event_str, &event_state);
		fprintf(stderr, "%s() recv event=[%s(%s)] ret=(%d)\n", __func__, event_str, event_state, ret);
		if (ret < 0) { 
			conn_status->assoc_state = -1;
			return;
		}
		goto RECV_MORE;
	} else if (recv_bytes > 0) {

		/* handle recv data */
		sctp_client_t *client = conn_status->sctp_client;
		msg.mtype = SCTP_MSG_RECV;
		sprintf(msg.tag.hostname, "%s", client->name);
		msg.tag.assoc_id = sinfo.sinfo_assoc_id;
		msg.tag.stream_id = sinfo.sinfo_stream;
		msg.tag.ppid = ntohl(sinfo.sinfo_ppid);

		relay_msg_to_ppid(&msg);

		goto RECV_MORE;
	} else if (recv_bytes < 0 && errno != EAGAIN) {

		/* handle error */
		fprintf(stderr, "error occured! (%d:%s)\n", errno, strerror(errno));
		conn_status->assoc_state = -1;
	}
}

void clear_connection(conn_status_t *conn_status)
{
	fprintf(stderr, "{dbg} %s() called!\n", __func__);

	/* clear resource */
	if (!TAILQ_EMPTY(&conn_status->queue_head)) {
		tailq_entry *item_now = TAILQ_FIRST(&conn_status->queue_head), *item_next = NULL;
		while (item_now != NULL) {
			item_next = TAILQ_NEXT(item_now, items); free(item_now); item_now = item_next;
			/* TODO : something more */
		}
	}
	TAILQ_INIT(&conn_status->queue_head);

	if (conn_status->conns_fd) {
		close(conn_status->conns_fd);
		conn_status->conns_fd = 0;
	}
	conn_status->assoc_id = 0;
	conn_status->assoc_state = 0;

	if (conn_status->conn_ev) {
		event_free(conn_status->conn_ev);
		conn_status->conn_ev = NULL;
	}
}

conn_status_t *search_connection(sctp_client_t *client, sctp_msg_t *sctp_msg)
{
	for (int i = 0; i < client->conn_num; i++) {
		conn_status_t *conn = &client->conn_status[(client->curr_index + i) % client->conn_num];
		if (sctp_msg->tag.assoc_id > 0 && sctp_msg->tag.assoc_id != conn->assoc_id) continue;
		if (conn->assoc_state != SCTP_ESTABLISHED) continue;
		client->curr_index = (i + 1) % client->conn_num;
		return conn;
	}
	return NULL;
}

void check_path_state(sctp_client_t *client)
{
	struct sctp_paddrinfo pinfo;
	socklen_t optlen = sizeof(pinfo);
	memset(&pinfo, 0x00, sizeof(struct sctp_paddrinfo));

	char peer_host[NI_MAXHOST] = {0,};
	char peer_port[NI_MAXSERV] = {0,};

	for (int i = 0; i < client->conn_num; i++) {
		conn_status_t *conn = &client->conn_status[i];
		pinfo.spinfo_assoc_id = conn->assoc_id;

		for (int k = 0; k < client->dst_addr_num; k++) {
			memcpy(&pinfo.spinfo_address, &client->dst_addr[k], sizeof(struct sockaddr_storage));

			int ret = sctp_opt_info(conn->conns_fd, conn->assoc_id, SCTP_GET_PEER_ADDR_INFO, &pinfo, &optlen);
			if (ret < 0) {
				//fprintf(stderr, "fail to get paddr\n"); // path goaway
			} else {
				getnameinfo((struct sockaddr *)&pinfo.spinfo_address, sizeof(struct sockaddr_storage), 
						peer_host, sizeof(peer_host), peer_port, sizeof(peer_port), NI_NUMERICHOST | NI_NUMERICSERV);
				fprintf(stderr, "{dbg} %s:%s path_state=(%d:%s)\n", peer_host, peer_port, pinfo.spinfo_state, get_path_state_str(pinfo.spinfo_state));
			}
		}
	}
}

void clear_or_reconnect(sctp_client_t *client, worker_ctx_t *worker_ctx)
{
	for (int i = 0; i < MAX_SC_CONN_NUM; i++) {
		if (client->conn_status[i].assoc_state > 0) {
			if (!client->enable || i >= client->conn_num) {
				clear_connection(&client->conn_status[i]);
			} 
		} else {
			if (client->enable && i < client->conn_num) {
				client_new(&client->conn_status[i], worker_ctx, client);
			}
		}
	}
}

void check_connection(worker_ctx_t *worker_ctx)
{
	conn_curr_t *CURR_CONN = take_conn_list(MAIN_CTX);

	for (int i = 0; i < link_node_length(&worker_ctx->conn_list); i++) {
		sctp_client_t *client = link_node_get_nth_data(&worker_ctx->conn_list, i);
		if (client == NULL) {
			continue;
		}

		/* publish conn status to SHM */
		for (int k = 0; k < MAX_SC_CONN_NUM; k++) {
			conn_status_t *conn_status = &CURR_CONN->conn_info[client->id].conn_status[k];
			if (k <= client->conn_num) {
				conn_status->conns_fd = client->conn_status[k].conns_fd;
				conn_status->assoc_id = client->conn_status[k].assoc_id;
				conn_status->assoc_state = client->conn_status[k].assoc_state;
			} else {
				memset(conn_status, 0x00, sizeof(conn_status_t));
			}
		}
		check_path_state(client);
		clear_or_reconnect(client, worker_ctx);
	}
}

void client_new(conn_status_t *conn_status, worker_ctx_t *worker_ctx, sctp_client_t *client)
{
	/* reset connection */
	clear_connection(conn_status);

	/* ok try reconnect */
	conn_status->conns_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	conn_status->sctp_client = client;
	evutil_make_socket_nonblocking(conn_status->conns_fd);

	setsockopt(conn_status->conns_fd, IPPROTO_SCTP, SCTP_EVENTS, &SCTP_NOTIF_EVENT, sizeof(SCTP_NOTIF_EVENT));
	setsockopt(conn_status->conns_fd, IPPROTO_SCTP, SCTP_INITMSG, &SCTP_INIT_MESSAGE, sizeof(SCTP_INIT_MESSAGE));
	setsockopt(conn_status->conns_fd, IPPROTO_SCTP, SCTP_PEER_ADDR_PARAMS, &SCTP_PADDR_PARAMS, sizeof(SCTP_PADDR_PARAMS));
	util_set_linger(conn_status->conns_fd, 1, 0);	/* we use ABORT */

	conn_status->conn_ev = event_new(worker_ctx->evbase_thrd, conn_status->conns_fd, EV_READ|EV_PERSIST, handle_sock_recv, conn_status);
	event_add(conn_status->conn_ev, NULL);

	sctp_bindx(conn_status->conns_fd, (struct sockaddr *)client->src_addr, client->src_addr_num, SCTP_BINDX_ADD_ADDR);
	sctp_connectx(conn_status->conns_fd, (struct sockaddr *)client->dst_addr, client->dst_addr_num, &conn_status->assoc_id);
}

void create_client(worker_ctx_t *worker_ctx, conn_info_t *conn)
{
	link_node *node = link_node_assign_key_order(&worker_ctx->conn_list, conn->name, sizeof(sctp_client_t));
	sctp_client_t *client = (sctp_client_t *)node->data;

	client->id = conn->id;
	sprintf(client->name, "%.127s", conn->name);
	client->enable = conn->enable;
	client->conn_num = conn->conn_num;

	/* address setting */
	for (int i = 0; i < MAX_SC_ADDR_NUM; i++) {
		if (strlen(conn->src_addr[i])) {
			client->src_addr_num++;
			client->src_addr[i].sin_family = AF_INET;
			client->src_addr[i].sin_addr.s_addr = inet_addr(conn->src_addr[i]);
			client->src_addr[i].sin_port = htons(conn->sport);
		}
		if (strlen(conn->dst_addr[i])) {
			client->dst_addr_num++;
			client->dst_addr[i].sin_family = AF_INET;
			client->dst_addr[i].sin_addr.s_addr = inet_addr(conn->dst_addr[i]);
			client->dst_addr[i].sin_port = htons(conn->dport);
		}
	}

	/* conn num trigger */
	for (int i = 0; i < client->conn_num && i < MAX_SC_CONN_NUM; i++) {
		if (client->enable) {
			client_new(&client->conn_status[i], worker_ctx, client);
		}
	}
}

void io_worker_init(worker_ctx_t *worker_ctx, main_ctx_t *MAIN_CTX)
{
	conn_curr_t *CURR_CONN = take_conn_list(MAIN_CTX);

	for (int i = 0; i < MAX_SC_CONN_LIST; i++) {
		conn_info_t *conn = &CURR_CONN->conn_info[i];
		if (conn->occupied == 0) continue;
		if (conn->id % MAIN_CTX->IO_WORKERS.worker_num != worker_ctx->thread_index) continue;

		create_client(worker_ctx, conn);
	}
}

void io_thrd_tick(evutil_socket_t fd, short events, void *data)
{
	worker_ctx_t *worker_ctx = (worker_ctx_t *)data;
	check_connection(worker_ctx);
}

void *io_worker_thread(void *arg)
{
	WORKER_CTX = (worker_ctx_t *)arg;
	WORKER_CTX->evbase_thrd = event_base_new();

	io_worker_init(WORKER_CTX, MAIN_CTX);

	struct timeval one_sec = {2, 0};
	struct event *ev_tick = event_new(WORKER_CTX->evbase_thrd, -1, EV_PERSIST, io_thrd_tick, WORKER_CTX);
	event_add(ev_tick, &one_sec);

	event_base_loop(WORKER_CTX->evbase_thrd, EVLOOP_NO_EXIT_ON_EMPTY);

	return NULL;
}
