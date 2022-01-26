#include "sctp_c.h"

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
	.sctp_sender_dry_event = 1,
	.sctp_stream_reset_event = 1,
	.sctp_assoc_reset_event = 1,
	.sctp_stream_change_event = 1,
	.sctp_send_failure_event_event = 1,
};

static struct sctp_initmsg SCTP_INIT_MESSAGE = {
	.sinit_num_ostreams = 5,
	.sinit_max_instreams = 5,
	.sinit_max_attempts = 4,
	/* .sinit_max_init_timeo = 0 */
};

void handle_sock_cb(int conn_fd, short events, void *data)
{
	(void)events;

	conn_status_t *conn_status = (conn_status_t *)data;

	struct sctp_sndrcvinfo sinfo = {0,};
	char buffer[65536] = {0,};
	struct sockaddr from = {0,};
	socklen_t from_len = sizeof(from);
	int recv_bytes = 0;
	int msg_flags = 0;

RECV_MORE:
	msg_flags = 0 | MSG_DONTWAIT;
	recv_bytes = sctp_recvmsg(conn_fd, buffer, 65536, &from, &from_len, &sinfo, &msg_flags);

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
	} if (recv_bytes < 0 && errno != EAGAIN) {

		/* handle error */
		fprintf(stderr, "error occured! (%d:%s)\n", errno, strerror(errno));
		conn_status->assoc_state = -1;
	} else if (recv_bytes > 0) {

		/* handle recv data */
		fprintf(stderr, "recv data=[%s]\n", buffer);
		goto RECV_MORE;
	}
}

void clear_connection(conn_status_t *conn_status)
{
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

void check_connection(worker_ctx_t *worker_ctx)
{
	for (int i = 0; i < link_node_length(&worker_ctx->conn_list); i++) {
		sctp_client_t *client = link_node_get_nth_data(&worker_ctx->conn_list, i);
		if (client == NULL) continue;

		for (int k = 0; k < MAX_SC_CONN_NUM; k++) {
			if (client->conn_status[k].assoc_state > 0) {
				if (!client->enable || k >= client->conn_num) {
					clear_connection(&client->conn_status[k]);
				} 
			} else {
				if (client->enable && k < client->conn_num) {
					client_new(&client->conn_status[k], worker_ctx, client);
				}
			}
		}
	}
}

void client_new(conn_status_t *conn_status, worker_ctx_t *worker_ctx, sctp_client_t *client)
{
	clear_connection(conn_status);

	/* ok try reconnect */
	conn_status->conns_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	evutil_make_socket_nonblocking(conn_status->conns_fd);

	setsockopt(conn_status->conns_fd, IPPROTO_SCTP, SCTP_EVENTS, &SCTP_NOTIF_EVENT, sizeof(SCTP_NOTIF_EVENT));
	setsockopt(conn_status->conns_fd, IPPROTO_SCTP, SCTP_INITMSG, &SCTP_INIT_MESSAGE, sizeof(SCTP_INIT_MESSAGE));
	util_set_linger(conn_status->conns_fd, 1, 0);	/* we use ABORT */

	conn_status->conn_ev = event_new(worker_ctx->evbase_thrd, conn_status->conns_fd, EV_READ|EV_PERSIST, handle_sock_cb, conn_status);
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
	worker_ctx->evbase_thrd = event_base_new();

	conn_curr_t *CURR_CONN = take_conn_list(MAIN_CTX);

	for (int i = 0; i < MAX_SC_CONN_LIST; i++) {
		conn_info_t *conn = &CURR_CONN->conn_info[i];
		if (conn->occupied == 0) continue;
		if (conn->id % MAIN_CTX->IO_WORKERS.worker_num != worker_ctx->thread_index) continue;

		create_client(worker_ctx, conn);
	}
}

void thrd_tick(evutil_socket_t fd, short events, void *data)
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
	struct event *ev_tick = event_new(WORKER_CTX->evbase_thrd, -1, EV_PERSIST, thrd_tick, WORKER_CTX);
	event_add(ev_tick, &one_sec);

	event_base_loop(WORKER_CTX->evbase_thrd, EVLOOP_NO_EXIT_ON_EMPTY);

	return NULL;
}
