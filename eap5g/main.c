#include <eapp.h>

char mySysName[COMM_MAX_NAME_LEN];
char myAppName[COMM_MAX_NAME_LEN];

main_ctx_t main_ctx, *MAIN_CTX = &main_ctx;

int create_worker_thread(worker_thread_t *WORKER, const char *prefix, main_ctx_t *MAIN_CTX)
{
    if (WORKER->worker_num <= 0 || WORKER->worker_num > MAX_WORKER_NUM) {
        fprintf(stderr, "%s() fatal! [%s] worker_num=(%d/max=%d)!\n", __func__, prefix, WORKER->worker_num, MAX_WORKER_NUM);
        return (-1);
    }

    WORKER->workers = calloc(1, sizeof(worker_ctx_t) * WORKER->worker_num);

    for (int i = 0; i < WORKER->worker_num; i++) {
        worker_ctx_t *worker_ctx = &WORKER->workers[i];
        worker_ctx->thread_index = i;
        sprintf(worker_ctx->thread_name, "%s_%02d", prefix, i);

        if (pthread_create(&worker_ctx->pthread_id, NULL, io_worker_thread, worker_ctx)) {
            fprintf(stderr, "%s() fatal! fail to create thread=(%s)\n", __func__, worker_ctx->thread_name);
            return (-1);
        }

		worker_ctx->udp_sock = create_udp_sock(0);

        pthread_setname_np(worker_ctx->pthread_id, worker_ctx->thread_name);
        fprintf(stderr, "setname=[%s]\n", worker_ctx->thread_name);
    }

    return 0;
}

int initialize(main_ctx_t *MAIN_CTX)
{
	//  
	sprintf(mySysName, "%.15s", getenv("MY_SYS_NAME"));
	sprintf(myAppName, "%.15s", "EAP5G"); 
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

	/* load config */
	config_init(&MAIN_CTX->CFG);
	if (!config_read_file(&MAIN_CTX->CFG, "./eap5g.cfg")) {
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
	}

    /* create queue id info */
    int queue_key = 0;
    config_lookup_int(&MAIN_CTX->CFG, "queue_id_info.eap5g_nwucp_queue", &queue_key);
    if ((MAIN_CTX->QID_INFO.eap5g_nwucp_qid = util_get_queue_info(queue_key, "eap5g_nwucp_queue")) < 0) {
        return (-1);
    }
    config_lookup_int(&MAIN_CTX->CFG, "queue_id_info.nwucp_eap5g_queue", &queue_key);
    if ((MAIN_CTX->QID_INFO.nwucp_eap5g_qid = util_get_queue_info(queue_key, "nwucp_eap5g_queue")) < 0) {
        return (-1);
    }
    /* create distr info */
    if (config_lookup_int(&MAIN_CTX->CFG, "distr_info.nwucp_worker_num", &MAIN_CTX->DISTR_INFO.worker_num) < 0 ||
            MAIN_CTX->DISTR_INFO.worker_num > MAX_WORKER_NUM) {
        return (-1);
    }   
    config_lookup_int(&MAIN_CTX->CFG, "distr_info.eap5g_nwucp_worker_queue", &queue_key);
    if ((MAIN_CTX->DISTR_INFO.worker_distr_qid = util_get_queue_info(queue_key, "eap5g_nwucp_worker_queue")) < 0) {
        return (-1);
    }

	/* udp listen port */
	if (config_lookup_int(&MAIN_CTX->CFG, "process_config.udp_listen_port", &MAIN_CTX->udp_listen_port) < 0) {
		return (-1);
	}
	if (udpfromto_init((MAIN_CTX->udp_sock = create_udp_sock(MAIN_CTX->udp_listen_port))) < 0) {
		fprintf(stderr, "%s() fail to create udp sock (port:%d) err(%d:%s)!\n", __func__, MAIN_CTX->udp_listen_port, errno, strerror(errno));
		return (-1);
	}
	fprintf(stderr, "%s() create udp_sock (port:%d fd:%d) success.\n", __func__, MAIN_CTX->udp_listen_port, MAIN_CTX->udp_sock);

	/* main udp buffer queue */
	config_lookup_int(&MAIN_CTX->CFG, "process_config.queue_size_udp", &MAIN_CTX->udp_buff.each_size);
	config_lookup_int(&MAIN_CTX->CFG, "process_config.queue_count_udp", &MAIN_CTX->udp_buff.total_num);

	if ((MAIN_CTX->udp_buff.each_size <= 0 || MAIN_CTX->udp_buff.each_size > MAX_DISTR_BUFF_SIZE) || 
		(MAIN_CTX->udp_buff.total_num <= 0 || MAIN_CTX->udp_buff.total_num > MAX_DISTR_BUFF_NUM)) {
		fprintf(stderr, "%s() invalid buffer queue_size_udp=(%d) or queue_count_udp=(%d)!\n", 
				__func__, MAIN_CTX->udp_buff.each_size, MAIN_CTX->udp_buff.total_num);
		return (-1);
	} else {
		MAIN_CTX->udp_buff.buffers = malloc(sizeof(recv_buf_t) * MAIN_CTX->udp_buff.total_num);
		for (int i = 0; i < MAIN_CTX->udp_buff.total_num; i++) {
			recv_buf_t *buffer = &MAIN_CTX->udp_buff.buffers[i];
			buffer->occupied = 0;
			buffer->size = 0;
			buffer->buffer = malloc(MAIN_CTX->udp_buff.each_size);
		}
	}

	/* ike listen port */
	if (config_lookup_int(&MAIN_CTX->CFG, "process_config.ike_listen_port", &MAIN_CTX->ike_listen_port) < 0) {
		return (-1);
	}

	/* io worker thread create */
	config_lookup_int(&MAIN_CTX->CFG, "process_config.io_worker_num", &MAIN_CTX->IO_WORKERS.worker_num);
	if (create_worker_thread(&MAIN_CTX->IO_WORKERS, "io_worker", MAIN_CTX) < 0) {
		return (-1);
	}

	/* all success */
	return 0;
}

int main()
{
	evthread_use_pthreads();

	MAIN_CTX->evbase_main = event_base_new();

	if (initialize(MAIN_CTX) < 0) {
		exit(0);
	}

	/* udp read -> worker~RR */
	struct event *ev_udp_read = event_new(MAIN_CTX->evbase_main, MAIN_CTX->udp_sock, EV_READ | EV_PERSIST, udp_sock_read_callback, MAIN_CTX);
	if (event_add(ev_udp_read, NULL) == -1) {
		fprintf(stderr, "%s() fail to create udp_read event\n", __func__);
		exit(0);
	}

	event_base_loop(MAIN_CTX->evbase_main, EVLOOP_NO_EXIT_ON_EMPTY);

	event_base_free(MAIN_CTX->evbase_main);

	return 0;
}
