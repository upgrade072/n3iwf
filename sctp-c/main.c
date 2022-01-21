#include "sctp_c.h"

main_ctx_t main_ctx, *MAIN_CTX = &main_ctx;

void create_client(worker_ctx_t *worker_ctx, conn_info_t *conn)
{
	link_node *node = link_node_assign_key_order(&worker_ctx->conn_list, conn->name, sizeof(sctp_client_t));
	sctp_client_t *client = (sctp_client_t *)node->data;

	client->id = conn->id;
	sprintf(client->name, "%.127s", conn->name);
	client->enable = conn->enable;
	client->conn_num = conn->conn_num;

	for (int i = 0; i < MAX_SC_ADDR_NUM; i++) {
		// TODO
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

	io_worker_init(worker_ctx, MAIN_CTX);

	worker_ctx->evbase_thrd = event_base_new();

	while (1) {
		sleep(1);
	}
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

	WORKER->workers = malloc(sizeof(worker_ctx_t) * WORKER->worker_num);

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
