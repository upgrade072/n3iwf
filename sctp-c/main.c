#include "sctp_c.h"

main_ctx_t main_ctx, *MAIN_CTX = &main_ctx;

int create_worker_recv_queue(worker_ctx_t *worker_ctx, main_ctx_t *MAIN_CTX)
{
	config_lookup_int(&MAIN_CTX->CFG, "process_config.queue_size_at_bf", &worker_ctx->recv_buff.each_size);
	config_lookup_int(&MAIN_CTX->CFG, "process_config.queue_count_at_bf", &worker_ctx->recv_buff.total_num);
	if ((worker_ctx->recv_buff.each_size <= 0 || worker_ctx->recv_buff.each_size > MAX_SCTP_BUFF_SIZE) ||
		(worker_ctx->recv_buff.total_num <= 0 || worker_ctx->recv_buff.total_num > MAX_SCTP_BUFF_NUM)) {
		fprintf(stderr, "%s() fatal! process_config.queue_size_at_bf=(%d/max=%d) process_config.queue_count_at_bf=(%d/max=%d)!\n",
				__func__, 
				worker_ctx->recv_buff.each_size, MAX_SCTP_BUFF_SIZE,
				worker_ctx->recv_buff.total_num, MAX_SCTP_BUFF_NUM);
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

	// todo ... //
	while (1) {
		sleep (1);

		//int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);

		sctp_msg_t send_msg;
		send_msg.mtype = 1;
		memset(&send_msg.tag, 0x00, sizeof(sctp_tag_t));
		sprintf(send_msg.tag.hostname, "%s", "test_conn_1");
		sprintf(send_msg.msg_body, "123456");
		send_msg.msg_size = strlen(send_msg.msg_body);

		int ret = msgsnd(MAIN_CTX->QID_INFO.send_relay, &send_msg, sizeof(sctp_tag_t) + sizeof(int) + sizeof(size_t) + send_msg.msg_size, IPC_NOWAIT);
		fprintf(stderr, "{dbg} ret=(%d) (%d:%s)\n", ret, errno, strerror(errno));

		//MAIN_CTX->QID_INFO.send_relay
	}
}
