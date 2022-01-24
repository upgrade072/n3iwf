#include "sctp_c.h"

main_ctx_t main_ctx, *MAIN_CTX = &main_ctx;

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

void client_read_cb(struct bufferevent *bev, void *data)
{
	fprintf(stderr, "%s called!\n", __func__);

	conn_status_t *conn_status = (conn_status_t *)data;
	int sock_fd = bufferevent_getfd(bev);
	fprintf(stderr, "fd=(%d) fd=(%d)\n", conn_status->conns_fd, sock_fd);

	struct sctp_sndrcvinfo sinfo = {0,};
	char buffer[65536] = {0,};
	int msg_flags = MSG_DONTWAIT;
	struct sockaddr from = {0,};
	socklen_t from_len = sizeof(from);

#if 0
	int recv_bytes = sctp_recvmsg(sock_fd, buffer, 65536, &from, &from_len, &sinfo, &msg_flags);

	if (msg_flags & MSG_NOTIFICATION) {
		fprintf(stderr, "recv notification!\n");
	} else {
		fprintf(stderr, "recv (%d) bytes! (%d:%s)\n", recv_bytes, errno, strerror(errno));
	}
#else
	ssize_t rcv_len = bufferevent_read(bev, buffer, 65535);
	fprintf(stderr, "rcv_len=(%ld) [%s]\n", rcv_len, buffer);
#endif
}

void client_write_cb(struct bufferevent *bev, void *data)
{
	fprintf(stderr, "%s called!\n", __func__);
}

void client_event_cb(struct bufferevent *bev, short events, void *data)
{
	fprintf(stderr, "%s called! ", __func__);

	if (events & /* peer shutdown/abort */ BEV_EVENT_EOF) {
		fprintf(stderr, "eof\n");
	} else if (events & /* connection failed */ BEV_EVENT_ERROR) {
		fprintf(stderr, "error\n");
	} else if (events & /* connected */ BEV_EVENT_CONNECTED) {
		printf(stderr, "connected\n");
	} else {
		fprintf(stderr, "unknown event=(%d)\n", events);
	}
}

void test_code(int conn_fd, short events, void *data)
{
	conn_status_t *conn_status = (conn_status_t *)data;

	struct sctp_sndrcvinfo sinfo = {0,};
	char buffer[65536] = {0,};
	int msg_flags = MSG_DONTWAIT;
	struct sockaddr from = {0,};
	socklen_t from_len = sizeof(from);
	int recv_bytes = 0;

RETRY:
	recv_bytes = sctp_recvmsg(conn_fd, buffer, 65536, &from, &from_len, &sinfo, &msg_flags);

	if (msg_flags & MSG_NOTIFICATION) {
		fprintf(stderr, "recv notification!\n");
	} else {
		fprintf(stderr, "recv (%d) bytes! (%d:%s)\n", recv_bytes, errno, strerror(errno));
	}

	if (recv_bytes > 0) goto RETRY;
}

void create_client(worker_ctx_t *worker_ctx, conn_info_t *conn)
{
	link_node *node = link_node_assign_key_order(&worker_ctx->conn_list, conn->name, sizeof(sctp_client_t));
	sctp_client_t *client = (sctp_client_t *)node->data;

	client->id = conn->id;
	sprintf(client->name, "%.127s", conn->name);
	client->enable = conn->enable;
	client->conn_num = conn->conn_num;

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

	if (client->enable) {
		for (int i = 0; i < client->enable && i < MAX_SC_CONN_NUM; i++) {
			conn_status_t *conn_status = &client->conn_status[i];
			conn_status->conns_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
			evutil_make_socket_nonblocking(conn_status->conns_fd);

			sctp_bindx(conn_status->conns_fd, (struct sockaddr *)client->src_addr, client->src_addr_num, SCTP_BINDX_ADD_ADDR);
			setsockopt(conn_status->conns_fd, IPPROTO_SCTP, SCTP_EVENTS, &SCTP_NOTIF_EVENT, sizeof(SCTP_NOTIF_EVENT));
			setsockopt(conn_status->conns_fd, IPPROTO_SCTP, SCTP_INITMSG, &SCTP_INIT_MESSAGE, sizeof(SCTP_INIT_MESSAGE));
			util_set_linger(conn_status->conns_fd, 1, 0);	/* we use ABORT */

			event_set(&conn_status->conn_ev, conn_status->conns_fd, EV_READ|EV_PERSIST, test_code, conn_status);
			event_base_set(worker_ctx->evbase_thrd, &conn_status->conn_ev);
			event_add(&conn_status->conn_ev, NULL);
			sctp_connectx(conn_status->conns_fd, (struct sockaddr *)client->dst_addr, client->dst_addr_num, &conn_status->assoc_id);
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

void *io_worker_thread(void *arg)
{
	worker_ctx_t *worker_ctx = (worker_ctx_t *)arg;
	worker_ctx->evbase_thrd = event_base_new();

	io_worker_init(worker_ctx, MAIN_CTX);

	event_base_loop(worker_ctx->evbase_thrd, EVLOOP_NO_EXIT_ON_EMPTY);

	return NULL;
}

void *bf_worker_thread(void *arg)
{
	while (1) {
		sleep(1);
	}
}

int create_worker_recv_queue(worker_ctx_t *worker_ctx, main_ctx_t *MAIN_CTX)
{
	config_lookup_int(&MAIN_CTX->CFG, "process_config.queue_size_at_bf", &worker_ctx->recv_buff.each_size);
	config_lookup_int(&MAIN_CTX->CFG, "process_config.queue_count_at_bf", &worker_ctx->recv_buff.total_num);
	if ((worker_ctx->recv_buff.each_size <= 0 || worker_ctx->recv_buff.each_size > MAX_MAIN_BUFF_SIZE) ||
		(worker_ctx->recv_buff.total_num <= 0 || worker_ctx->recv_buff.total_num > MAX_MAIN_BUFF_NUM)) {
		fprintf(stderr, "%s() fatal! process_config.queue_size_at_bf=(%d/max=%d) process_config.queue_count_at_bf=(%d/max=%d)!\n",
				__func__, 
				worker_ctx->recv_buff.each_size, MAX_MAIN_BUFF_SIZE,
				worker_ctx->recv_buff.total_num, MAX_MAIN_BUFF_NUM);
		return (-1);
	}

	worker_ctx->recv_buff.buffers = malloc(sizeof(recv_buf_t) * worker_ctx->recv_buff.total_num);
	for (int i = 0; i < worker_ctx->recv_buff.total_num; i++) {
		recv_buf_t *buffer = &worker_ctx->recv_buff.buffers[i];
		buffer->occupied = 0;
		buffer->size = 0;
		buffer->buffer = malloc(worker_ctx->recv_buff.each_size);
	}

	return (0);
}

int create_worker_thread(worker_thread_t *WORKER, const char *prefix, main_ctx_t *MAIN_CTX)
{
	int io_worker = 0, bf_worker = 0;
	if (!strcmp(prefix, "io_worker")) {
		io_worker = 1;
	} else if (!strcmp(prefix, "bf_worker")) {
		bf_worker = 1;
	}
	if (!io_worker && !bf_worker) {
		fprintf(stderr, "%s() fatal! recv unhandle worker request=[%s]\n", __func__, prefix);
		return (-1);
	}
	if (WORKER->worker_num <= 0 || WORKER->worker_num > MAX_WORKER_NUM) {
		fprintf(stderr, "%s() fatal! [%s] worker_num=(%d/max=%d)!\n", __func__, prefix, WORKER->worker_num, MAX_WORKER_NUM);
		return (-1);
	}

	WORKER->workers = calloc(1, sizeof(worker_ctx_t) * WORKER->worker_num);

	for (int i = 0; i < WORKER->worker_num; i++) {
		worker_ctx_t *worker_ctx = &WORKER->workers[i];
		worker_ctx->thread_index = i;
		sprintf(worker_ctx->thread_name, "%s_%02d", prefix, i);

		if (bf_worker == 1 && create_worker_recv_queue(worker_ctx, MAIN_CTX) < 0) {
			fprintf(stderr, "%s() fatal! fail to recv queue at worker=[%s]\n", __func__, worker_ctx->thread_name);
			return (-1);
		}

		if (pthread_create(&worker_ctx->pthread_id, NULL, 
					io_worker ? io_worker_thread : bf_worker_thread,
					worker_ctx)) {
			fprintf(stderr, "%s() fatal! fail to create thread=(%s)\n", __func__, worker_ctx->thread_name);
			return (-1);
		}

		pthread_setname_np(worker_ctx->pthread_id, worker_ctx->thread_name);
		fprintf(stderr, "setname=[%s]\n", worker_ctx->thread_name);
	}

	return 0;
}

conn_curr_t *take_conn_list(main_ctx_t *MAIN_CTX)
{
	conn_curr_t *CURR_CONN = &MAIN_CTX->SHM_SCTPC_CONN->curr_conn[MAIN_CTX->SHM_SCTPC_CONN->curr_pos];

	return CURR_CONN;
}

int sort_conn_list(const void *a, const void *b)
{
	const conn_info_t *v_a = a;
	const conn_info_t *v_b = b;
	return (strcmp(v_a->name, v_b->name));
}

void disp_conn_list(main_ctx_t *MAIN_CTX)
{
	conn_curr_t *CURR_CONN = &MAIN_CTX->SHM_SCTPC_CONN->curr_conn[MAIN_CTX->SHM_SCTPC_CONN->curr_pos];
	for (int i = 0; i < CURR_CONN->rows_num; i++) {
		conn_info_t *conn = &CURR_CONN->dist_info[i];
		fprintf(stderr, "{dbg} %s id=(%d) name=(%s)\n", __func__, conn->id, conn->name);
	}
}

void init_conn_list(main_ctx_t *MAIN_CTX)
{
	config_setting_t *conf_conn_list = config_lookup(&MAIN_CTX->CFG, "connection_list");
	int conn_list_num = conf_conn_list == NULL ? 0 : config_setting_length(conf_conn_list);
	if (conn_list_num <= 0) {
		return;
	}

	int shm_prepare_pos = (MAIN_CTX->SHM_SCTPC_CONN->curr_pos + 1) % MAX_SC_CONN_SLOT;

	conn_curr_t *CURR_CONN = &MAIN_CTX->SHM_SCTPC_CONN->curr_conn[shm_prepare_pos];
	CURR_CONN->rows_num = 0;
	for (int i = 0; i < MAX_SC_CONN_LIST; i++) {
		CURR_CONN->conn_info[i].occupied = 0;
		CURR_CONN->dist_info[i].occupied = 0;
	}

	for (int i = 0; i < conn_list_num; i++) {
		config_setting_t *elem = config_setting_get_elem(conf_conn_list, i);

		/* prepare */
		int id = 0;
		const char *name = NULL;
		int enable = 0;
		int conn_num = 0;
		const char *src_ip_a = NULL;
		const char *src_ip_b = NULL;
		const char *src_ip_c = NULL;
		const char *dst_ip_a = NULL;
		const char *dst_ip_b = NULL;
		const char *dst_ip_c = NULL;
		int src_port = 0;
		int dst_port = 0;

		/* get from config */
		config_setting_lookup_int(elem, "id", &id);
		config_setting_lookup_string(elem, "name", &name);
		config_setting_lookup_int(elem, "enable", &enable);
		config_setting_lookup_int(elem, "conn_num", &conn_num);
		config_setting_lookup_string(elem, "src_ip_a", &src_ip_a);
		config_setting_lookup_string(elem, "src_ip_b", &src_ip_b);
		config_setting_lookup_string(elem, "src_ip_c", &src_ip_c);
		config_setting_lookup_string(elem, "dst_ip_a", &dst_ip_a);
		config_setting_lookup_string(elem, "dst_ip_b", &dst_ip_b);
		config_setting_lookup_string(elem, "dst_ip_c", &dst_ip_c);
		config_setting_lookup_int(elem, "src_port", &src_port);
		config_setting_lookup_int(elem, "dst_port", &dst_port);

		/* check value */
		if (id < 0 || id >= MAX_SC_CONN_LIST) {
			fprintf(stderr, "%s() check invalid id=(%d)\n", __func__, id);
			continue;
		} else if (CURR_CONN->conn_info[id].occupied == 1) {
			fprintf(stderr, "%s() check duplicated id=(%d)\n", __func__, id);
			continue;
		} else if (strlen(name) == 0 || strlen(name) >= 128) {
			fprintf(stderr, "%s() check invalid name_len(%ld) id=(%d)\n", __func__, strlen(name), id);
			continue;
		} else if (conn_num < 0 || conn_num >= MAX_SC_CONN_NUM) {
			fprintf(stderr, "%s() check invalid conn_num(%d) id=(%d)\n", __func__, conn_num, id);
			continue;
		}

		/* set to shm */
		conn_info_t *conn = &CURR_CONN->conn_info[id];
		conn->occupied = 1;
		conn->id = id; 
		sprintf(conn->name, "%.127s", name);
		conn->enable = enable;
		conn->conn_num = conn_num;
		sprintf(conn->src_addr[0], "%.127s", src_ip_a);
		sprintf(conn->src_addr[1], "%.127s", src_ip_b);
		sprintf(conn->src_addr[2], "%.127s", src_ip_c);
		sprintf(conn->dst_addr[0], "%.127s", dst_ip_a);
		sprintf(conn->dst_addr[1], "%.127s", dst_ip_b);
		sprintf(conn->dst_addr[2], "%.127s", dst_ip_c);
		conn->sport = src_port;
		conn->dport = dst_port;
		memset(conn->conn_status, 0x00, sizeof(conn_status_t) * MAX_SC_CONN_NUM);

		/* dist_info will sorting by name */
		memcpy(&CURR_CONN->dist_info[CURR_CONN->rows_num], conn, sizeof(conn_info_t));
		CURR_CONN->rows_num++;
	}

	/* sort for bf_thread */
	qsort(CURR_CONN->dist_info, CURR_CONN->rows_num, sizeof(conn_info_t), sort_conn_list);

	/* publish to shm with <curr_pos> */
	MAIN_CTX->SHM_SCTPC_CONN->curr_pos = shm_prepare_pos;
}

int initialize(main_ctx_t *MAIN_CTX)
{
	/* check sctp support */
	int sctp_test = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (sctp_test < 0) {
		fprintf(stderr, "%s() fatal! sctp not supported! run <checksctp>, run <modprobe sctp>\n", __func__);
		return (-1);
	} else {
		close(sctp_test);
	}

	/* loading config */
	config_init(&MAIN_CTX->CFG);
	if (!config_read_file(&MAIN_CTX->CFG, "./sctp_c.cfg")) {
		fprintf(stderr, "%s() fatal! fail to load cfg file=(%s) line:text(%d/%s)!\n",
			__func__,
			config_error_file(&MAIN_CTX->CFG),
			config_error_line(&MAIN_CTX->CFG),
			config_error_text(&MAIN_CTX->CFG));

		return (-1);
	} else {
		fprintf(stderr, "%s() load cfg ---------------------\n", __func__);
		config_write(&MAIN_CTX->CFG, stderr);
		fprintf(stderr, "===========================================\n");

		config_set_options(&MAIN_CTX->CFG, 0);
		config_set_tab_width(&MAIN_CTX->CFG, 4);
		config_write_file(&MAIN_CTX->CFG, "./sctp_c.cfg"); // save cfg with indent
	}

	/* create queue_id_info */
	int queue_key = 0;
	config_lookup_int(&MAIN_CTX->CFG, "queue_id_info.send_relay", &queue_key);
	if ((MAIN_CTX->QID_INFO.send_relay = util_get_queue_info(queue_key, "send_relay")) < 0) {
		return (-1);
	}
	config_setting_t *conf_recv_relay = config_lookup(&MAIN_CTX->CFG, "queue_id_info.recv_relay");
	int recv_relay_num = conf_recv_relay == NULL ? 0 : config_setting_length(conf_recv_relay);
	if (recv_relay_num <= 0) {
		fprintf(stderr, "%s() fatal! queue_id_info.recv_relay not exist!\n", __func__);
		return (-1);
	} else {
		MAIN_CTX->QID_INFO.recv_relay = malloc(sizeof(ppid_pqid_t) * recv_relay_num);
		for (int i = 0; i < recv_relay_num; i++) {
			config_setting_t *elem = config_setting_get_elem(conf_recv_relay, i);
			ppid_pqid_t *pqid = &MAIN_CTX->QID_INFO.recv_relay[i];
			config_setting_lookup_int(elem, "sctp_ppid", &pqid->sctp_ppid);
			config_setting_lookup_int(elem, "proc_pqid", &queue_key);
			if ((pqid->proc_pqid = util_get_queue_info(queue_key, "recv_relay")) < 0) {
				return (-1);
			}
		}
	}

	/* create connection list SHM */
	int shm_key = 0;
	config_lookup_int(&MAIN_CTX->CFG, "queue_id_info.sctp_conn_shm_key", &shm_key);
	if ((MAIN_CTX->SHM_SCTPC_CONN = util_get_shm(shm_key, sizeof(conn_list_t), "shm_sctpc_conn")) == NULL) {
		return (-1);
	}

	/* init connection list */
	init_conn_list(MAIN_CTX);
	disp_conn_list(MAIN_CTX);

	/* create io workers */
	config_lookup_int(&MAIN_CTX->CFG, "process_config.io_worker_num", &MAIN_CTX->IO_WORKERS.worker_num);
	if (create_worker_thread(&MAIN_CTX->IO_WORKERS, "io_worker", MAIN_CTX) < 0) {
		return (-1);
	}

	/* create bf worker */
	config_lookup_int(&MAIN_CTX->CFG, "process_config.bf_worker_num", &MAIN_CTX->BF_WORKERS.worker_num);
	if (create_worker_thread(&MAIN_CTX->BF_WORKERS, "bf_worker", MAIN_CTX) < 0) {
		return (-1);
	}

	return (0);
}

int main()
{
	evthread_use_pthreads();

	if (initialize(MAIN_CTX) < 0) {
		exit(0);
	}

	/* create main */
	MAIN_CTX->evbase_main = event_base_new();

	while (1) {
		sleep (1);
	}
}
