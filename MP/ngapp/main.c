#include "ngapp.h"

char mySysName[COMM_MAX_NAME_LEN];
char myAppName[COMM_MAX_NAME_LEN];

main_ctx_t main_ctx, *MAIN_CTX = &main_ctx;

int create_worker_recv_queue(worker_ctx_t *worker_ctx, main_ctx_t *MAIN_CTX)
{
    config_lookup_int(&MAIN_CTX->CFG, "process_config.queue_size_at_bf", &worker_ctx->recv_buff.each_size);
    config_lookup_int(&MAIN_CTX->CFG, "process_config.queue_count_at_bf", &worker_ctx->recv_buff.total_num);
    if ((worker_ctx->recv_buff.each_size <= 0 || worker_ctx->recv_buff.each_size > MAX_DISTR_BUFF_SIZE) ||
        (worker_ctx->recv_buff.total_num <= 0 || worker_ctx->recv_buff.total_num > MAX_DISTR_BUFF_NUM)) {
        ERRLOG(LLE, FL, "%s() fatal! process_config.queue_size_at_bf=(%d/max=%d) process_config.queue_count_at_bf=(%d/max=%d)!\n",
                __func__,
                worker_ctx->recv_buff.each_size, MAX_DISTR_BUFF_SIZE,
                worker_ctx->recv_buff.total_num, MAX_DISTR_BUFF_NUM);
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
	sprintf(myAppName, "%.15s", "NGAPP"); 
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

	/* load config */
    config_init(&MAIN_CTX->CFG);

	char tmp_path[1024] = {0,};
	sprintf(tmp_path, "%s/data/N3IWF_CONFIG/ngapp.cfg", getenv("IV_HOME"));
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

	/* create asn1 context */
	if (ossinit(&MAIN_CTX->world, NGAP_PDU_Descriptions)) {
		ERRLOG(LLE, FL, "Error: ossinit()\n");
		return(-1);
	} else {
		ossSetFlags(&MAIN_CTX->world, ossGetFlags(&MAIN_CTX->world) | AUTOMATIC_ENCDEC);
		ossSetDebugFlags(&MAIN_CTX->world, ossGetDebugFlags(&MAIN_CTX->world));
		MAIN_CTX->pdu_num = NGAP_PDU_PDU;
	}

	/* create queue id info */
    if ((MAIN_CTX->QID_INFO.main_qid = commlib_crteMsgQ(l_sysconf, "NGAPP", TRUE)) < 0) {
        ERRLOG(LLE, FL, "%s() fatal! queue_id_info.main_qid not exist!\n", __func__);
        return (-1);
    }
    if ((MAIN_CTX->QID_INFO.ixpc_qid = commlib_crteMsgQ(l_sysconf, "IXPC", TRUE)) < 0) {
        ERRLOG(LLE, FL, "%s() fatal! queue_id_info.ixpc_qid not exist!\n", __func__);
        return (-1);
    }
	int queue_key = 0;
	config_lookup_int(&MAIN_CTX->CFG, "queue_id_info.ngapp_sctpc_queue", &queue_key);
	if ((MAIN_CTX->QID_INFO.ngapp_sctpc_qid = util_get_queue_info(queue_key, "ngapp_sctpc_queue")) < 0) {
		return (-1);
	}
	config_lookup_int(&MAIN_CTX->CFG, "queue_id_info.sctpc_ngapp_queue", &queue_key);
	if ((MAIN_CTX->QID_INFO.sctpc_ngapp_qid = util_get_queue_info(queue_key, "sctpc_ngapp_queue")) < 0) {
		return (-1);
	}
	config_lookup_int(&MAIN_CTX->CFG, "queue_id_info.ngapp_nwucp_queue", &queue_key);
	if ((MAIN_CTX->QID_INFO.ngapp_nwucp_qid = util_get_queue_info(queue_key, "ngapp_nwucp_queue")) < 0) {
		return (-1);
	}
	config_lookup_int(&MAIN_CTX->CFG, "queue_id_info.nwucp_ngapp_queue", &queue_key);
	if ((MAIN_CTX->QID_INFO.nwucp_ngapp_qid = util_get_queue_info(queue_key, "nwucp_ngapp_queue")) < 0) {
		return (-1);
	}

	/* create distr info */
	if (config_lookup(&MAIN_CTX->CFG, "distr_info.ngap_distr_rule") == NULL) {
		return (-1);
	}
	if (config_lookup_int(&MAIN_CTX->CFG, "distr_info.nwucp_worker_num", &MAIN_CTX->DISTR_INFO.worker_num) < 0 || 
			MAIN_CTX->DISTR_INFO.worker_num > MAX_WORKER_NUM) {
		return (-1);
	}
	config_lookup_int(&MAIN_CTX->CFG, "distr_info.ngapp_nwucp_worker_queue", &queue_key);
	if ((MAIN_CTX->DISTR_INFO.worker_distr_qid = util_get_queue_info(queue_key, "ngapp_nwucp_worker_queue")) < 0) {
		return (-1);
	}

	/* create io workers */
	config_lookup_int(&MAIN_CTX->CFG, "process_config.io_worker_num", &MAIN_CTX->IO_WORKERS.worker_num);
	if (create_worker_thread(&MAIN_CTX->IO_WORKERS, "io_worker", MAIN_CTX) < 0) {
		return (-1);
	}
	MAIN_CTX->BF_WORKERS.worker_num = 2; /* ngap + sctp */
	if (create_worker_thread(&MAIN_CTX->BF_WORKERS, "bf_worker", MAIN_CTX) < 0) {
		return (-1);
	}

	return (0);
}

void main_tick(int conn_fd, short events, void *data)
{
	keepalivelib_increase();
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

	MAIN_CTX->evbase_main = event_base_new();

	if (initialize(MAIN_CTX) < 0) {
		exit(0);
	}

	struct timeval one_sec = {1, 0};
	struct event *ev_tick = event_new(MAIN_CTX->evbase_main, -1, EV_PERSIST, main_tick, NULL);
	event_add(ev_tick, &one_sec);

	struct timeval one_u_sec = {0,1};
	struct event *ev_msgq_read = event_new(MAIN_CTX->evbase_main, -1, EV_PERSIST, main_msgq_read_callback, MAIN_CTX);
	event_add(ev_msgq_read, &one_u_sec);

	event_base_loop(MAIN_CTX->evbase_main, EVLOOP_NO_EXIT_ON_EMPTY);

	return (0);
}
