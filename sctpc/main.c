#include "sctpc.h"

char mySysName[COMM_MAX_NAME_LEN];
char myAppName[COMM_MAX_NAME_LEN];

main_ctx_t main_ctx, *MAIN_CTX = &main_ctx;

int create_worker_recv_queue(worker_ctx_t *worker_ctx, main_ctx_t *MAIN_CTX)
{
	config_lookup_int(&MAIN_CTX->CFG, "process_config.queue_size_at_bf", &worker_ctx->recv_buff.each_size);
	config_lookup_int(&MAIN_CTX->CFG, "process_config.queue_count_at_bf", &worker_ctx->recv_buff.total_num);
	if ((worker_ctx->recv_buff.each_size <= 0 || worker_ctx->recv_buff.each_size > MAX_SCTP_BUFF_SIZE) ||
		(worker_ctx->recv_buff.total_num <= 0 || worker_ctx->recv_buff.total_num > MAX_SCTP_BUFF_NUM)) {
		ERRLOG(LLE, FL, "%s() fatal! process_config.queue_size_at_bf=(%d/max=%d) process_config.queue_count_at_bf=(%d/max=%d)!\n",
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
		ERRLOG(LLE, FL, "%s() fatal! recv unhandle worker request=[%s]\n", __func__, prefix);
		return (-1);
	}
	if (WORKER->worker_num <= 0 || WORKER->worker_num > MAX_WORKER_NUM) {
		ERRLOG(LLE, FL, "%s() fatal! [%s] worker_num=(%d/max=%d)!\n", __func__, prefix, WORKER->worker_num, MAX_WORKER_NUM);
		return (-1);
	}

	WORKER->workers = calloc(1, sizeof(worker_ctx_t) * WORKER->worker_num);

	for (int i = 0; i < WORKER->worker_num; i++) {
		worker_ctx_t *worker_ctx = &WORKER->workers[i];
		worker_ctx->thread_index = i;
		sprintf(worker_ctx->thread_name, "%s_%02d", prefix, i);

		if (bf_worker == 1 && create_worker_recv_queue(worker_ctx, MAIN_CTX) < 0) {
			ERRLOG(LLE, FL, "%s() fatal! fail to recv queue at worker=[%s]\n", __func__, worker_ctx->thread_name);
			return (-1);
		}

		if (pthread_create(&worker_ctx->pthread_id, NULL, 
					io_worker ? io_worker_thread : bf_worker_thread,
					worker_ctx)) {
			ERRLOG(LLE, FL, "%s() fatal! fail to create thread=(%s)\n", __func__, worker_ctx->thread_name);
			return (-1);
		}

		pthread_setname_np(worker_ctx->pthread_id, worker_ctx->thread_name);
		ERRLOG(LLE, FL, "setname=[%s]\n", worker_ctx->thread_name);
	}

	return 0;
}

int initialize(main_ctx_t *MAIN_CTX)
{
	//  
	sprintf(mySysName, "%.15s", getenv("MY_SYS_NAME"));
	sprintf(myAppName, "%.15s", "SCTPC"); 
	//  
	initLog(myAppName);
	// start with Log Level Error 
	loglib_setLogLevel(ELI, LLE);
	loglib_setLogLevel(DLI, LLE);
	loglib_setLogLevel(MLI, LLE);
	// let's start process 
	ERRLOG(LLE, FL, "Welcome ---------------\n");
	TRCLOG(LLE, FL, "Welcome ---------------\n");
	MSGLOG(LLE, FL, "Welcome ---------------\n");

	if (conflib_initConfigData() < 0) {
		ERRLOG(LLE, FL, "[%s.%d] conflib_initConfigData() Error\n", FL);
		return -1;
	}
	if (keepalivelib_init(myAppName) < 0) {
		ERRLOG(LLE, FL, "[%s.%d] keepalivelib_init() Error\n", FL);
		return -1;
	}

	/* check sctp support */
	int sctp_test = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (sctp_test < 0) {
		ERRLOG(LLE, FL, "%s() fatal! sctp not supported! run <checksctp>, run <modprobe sctp>\n", __func__);
		return (-1);
	} else {
		close(sctp_test);
	}

	/* loading config */
	config_init(&MAIN_CTX->CFG);

	char tmp_path[1024] = {0,};
	sprintf(tmp_path, "%s/data/N3IWF_CONFIG/sctpc.cfg", getenv("IV_HOME"));
	if (!config_read_file(&MAIN_CTX->CFG, tmp_path)) {
		ERRLOG(LLE, FL, "%s() fatal! fail to load cfg file=(%s) line:text(%d/%s)!\n",
			__func__,
			config_error_file(&MAIN_CTX->CFG),
			config_error_line(&MAIN_CTX->CFG),
			config_error_text(&MAIN_CTX->CFG));

		return (-1);
	} else {
		ERRLOG(LLE, FL, "%s() load cfg ---------------------\n", __func__);
		config_write(&MAIN_CTX->CFG, stderr);
		ERRLOG(LLE, FL, "===========================================\n");
	}

	/* create queue_id_info */
	if ((MAIN_CTX->QID_INFO.main_qid = commlib_crteMsgQ(l_sysconf, "SCTPC", TRUE)) < 0) {
		ERRLOG(LLE, FL, "%s() fatal! queue_id_info.main_qid not exist!\n", __func__);
		return (-1);
	}
	if ((MAIN_CTX->QID_INFO.ixpc_qid = commlib_crteMsgQ(l_sysconf, "IXPC", TRUE)) < 0) {
		ERRLOG(LLE, FL, "%s() fatal! queue_id_info.ixpc_qid not exist!\n", __func__);
		return (-1);
	}
	int queue_key = 0;
	config_lookup_int(&MAIN_CTX->CFG, "queue_id_info.sctp_send_relay", &queue_key);
	if ((MAIN_CTX->QID_INFO.sctp_send_relay = util_get_queue_info(queue_key, "sctp_send_relay")) < 0) {
		return (-1);
	}
	config_setting_t *conf_sctp_recv_relay = config_lookup(&MAIN_CTX->CFG, "queue_id_info.sctp_recv_relay");
	MAIN_CTX->QID_INFO.sctp_recv_relay_num = conf_sctp_recv_relay == NULL ? 0 : config_setting_length(conf_sctp_recv_relay);
	if (MAIN_CTX->QID_INFO.sctp_recv_relay_num <= 0) {
		ERRLOG(LLE, FL, "%s() fatal! queue_id_info.sctp_recv_relay not exist!\n", __func__);
		return (-1);
	} else {
		MAIN_CTX->QID_INFO.sctp_recv_relay = malloc(sizeof(ppid_pqid_t) * MAIN_CTX->QID_INFO.sctp_recv_relay_num);
		for (int i = 0; i < MAIN_CTX->QID_INFO.sctp_recv_relay_num; i++) {
			config_setting_t *elem = config_setting_get_elem(conf_sctp_recv_relay, i);
			ppid_pqid_t *pqid = &MAIN_CTX->QID_INFO.sctp_recv_relay[i];
			config_setting_lookup_int(elem, "sctp_ppid", &pqid->sctp_ppid);
			config_setting_lookup_int(elem, "proc_pqid", &queue_key);
			if ((pqid->proc_pqid = util_get_queue_info(queue_key, "sctp_recv_relay")) < 0) {
				ERRLOG(LLE, FL, "%s() fatal! queue_id_info.sctp_recv_relay not exist!\n", __func__);
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

void main_tick(evutil_socket_t fd, short events, void *data)
{
	keepalivelib_increase();

	disp_conn_list(MAIN_CTX);
	disp_conn_stat(MAIN_CTX);
}

void main_msgq_read_callback(evutil_socket_t fd, short events, void *data)
{
	main_ctx_t *MAIN_CTX = (main_ctx_t *)data;

	char recv_buffer[1024*64];
	GeneralQMsgType *msg = (GeneralQMsgType *)recv_buffer;
	int recv_size = 0;

	while ((recv_size = msgrcv(MAIN_CTX->QID_INFO.main_qid, msg, sizeof(recv_buffer), 0, IPC_NOWAIT)) >= 0) {
		switch(msg->mtype) {
			case MTYPE_SETPRINT:
				continue;
			case MTYPE_MMC_REQUEST:
				continue;
			case MTYPE_STATISTICS_REQUEST:
				send_conn_stat(MAIN_CTX, (IxpcQMsgType *)msg->body);
				continue;
			default:
				continue;
		}
	}
}

int main()
{
	evthread_use_pthreads();

	/* create main */
	MAIN_CTX->evbase_main = event_base_new();

	if (initialize(MAIN_CTX) < 0) {
		exit(0);
	} else {
		char cmd[10240] = {0,};
		sprintf(cmd, "touch %s/data/sctpc_start", getenv("IV_HOME"));
		system(cmd);
	}

    struct timeval one_sec = {1, 0};
    struct event *ev_tick = event_new(MAIN_CTX->evbase_main, -1, EV_PERSIST, main_tick, NULL);
    event_add(ev_tick, &one_sec);

	struct timeval one_u_sec = {0,1};
	struct event *ev_msgq_read = event_new(MAIN_CTX->evbase_main, -1, EV_PERSIST, main_msgq_read_callback, MAIN_CTX);
	event_add(ev_msgq_read, &one_u_sec);

    event_base_loop(MAIN_CTX->evbase_main, EVLOOP_NO_EXIT_ON_EMPTY);

	return 0;
}
