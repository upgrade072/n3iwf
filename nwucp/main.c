#include <nwucp.h>

char mySysName[COMM_MAX_NAME_LEN];
char myAppName[COMM_MAX_NAME_LEN];

main_ctx_t main_ctx, *MAIN_CTX = &main_ctx;

int load_config(config_t *CFG)
{
	/* load config */
	config_init(CFG);

	char tmp_path[1024] = {0,};
	sprintf(tmp_path, "%s/data/N3IWF_CONFIG/nwucp.cfg", getenv("IV_HOME"));
    if (!config_read_file(CFG, tmp_path)) {
        ERRLOG(LLE, FL, "cfg err!\n");
        ERRLOG(LLE, FL, "%s() fatal! fail to load cfg file=(%s) line:text(%d/%s)!\n",
                __func__,
                config_error_file(CFG),
                config_error_line(CFG),
                config_error_text(CFG));
		return (-1);
	}

	config_set_options(CFG, 0);
	config_write(CFG, stderr);

	return (0);
}

int create_n3iwf_profile(main_ctx_t *MAIN_CTX)
{
    /* cnvt whole profile to JSON format */
    json_object *json_cfg = json_object_new_object();
    config_setting_t *cfg_n3iwf_profile = config_lookup(&MAIN_CTX->CFG, "n3iwf_profile");
    cnvt_cfg_to_json(json_cfg, cfg_n3iwf_profile, CONFIG_TYPE_GROUP);

    /* load some info from CFG */
    const char *mcc_mnc = JS_SEARCH_STR(json_cfg, "/n3iwf_profile/global_info/mcc_mnc");
    int n3iwf_id = JS_SEARCH_INT(json_cfg, "/n3iwf_profile/global_info/n3iwf_id");
    const char *node_name = JS_SEARCH_STR(json_cfg, "/n3iwf_profile/global_info/node_name");
    json_object *js_support_ta_item = JS_SEARCH_OBJ(json_cfg, "/n3iwf_profile/SupportedTAItem");

    /* OK we get ngap_setup_request JSON PDU */
    MAIN_CTX->js_ng_setup_request = create_ng_setup_request_json(mcc_mnc, n3iwf_id, node_name, js_support_ta_item);

    ERRLOG(LLE, FL, "%s() create message\n%s\n", __func__, JS_PRINT_PRETTY(MAIN_CTX->js_ng_setup_request));

	json_object_put(json_cfg);

	return (0);
}

int create_qid_info(main_ctx_t *MAIN_CTX)
{
	if ((MAIN_CTX->QID_INFO.main_qid = commlib_crteMsgQ(l_sysconf, "NWUCP", TRUE)) < 0) {
		ERRLOG(LLE, FL, "%s() fatal! queue_id_info.main_qid not exist!\n", __func__);
		return (-1);
	}
	int queue_key = 0;
	config_lookup_int(&MAIN_CTX->CFG, "queue_id_info.ngapp_nwucp_queue", &queue_key);
	if ((MAIN_CTX->QID_INFO.ngapp_nwucp_qid = util_get_queue_info(queue_key, "ngapp_nwucp_queue")) < 0) {
		ERRLOG(LLE, FL, "%s() fatal! queue_id_info.ngapp_nwucp_queue not exist!\n", __func__);
		return (-1);
	}
	config_lookup_int(&MAIN_CTX->CFG, "queue_id_info.nwucp_ngapp_queue", &queue_key);
	if ((MAIN_CTX->QID_INFO.nwucp_ngapp_qid = util_get_queue_info(queue_key, "nwucp_ngapp_queue")) < 0) {
		ERRLOG(LLE, FL, "%s() fatal! queue_id_info.nwucp_ngapp_queue not exist!\n", __func__);
		return (-1);
	}
	config_lookup_int(&MAIN_CTX->CFG, "queue_id_info.eap5g_nwucp_queue", &queue_key);
	if ((MAIN_CTX->QID_INFO.eap5g_nwucp_qid = util_get_queue_info(queue_key, "eap5g_nwucp_queue")) < 0) {
		ERRLOG(LLE, FL, "%s() fatal! queue_id_info.eap5g_nwucp_queue not exist!\n", __func__);
		return (-1);
	}
	config_lookup_int(&MAIN_CTX->CFG, "queue_id_info.nwucp_eap5g_queue", &queue_key);
	if ((MAIN_CTX->QID_INFO.nwucp_eap5g_qid = util_get_queue_info(queue_key, "nwucp_eap5g_queue")) < 0) {
		ERRLOG(LLE, FL, "%s() fatal! queue_id_info.nwucp_eap5g_queue not exist!\n", __func__);
		return (-1);
	}
	config_lookup_int(&MAIN_CTX->CFG, "distr_info.ngapp_nwucp_worker_queue", &queue_key);
	if ((MAIN_CTX->QID_INFO.ngapp_nwucp_worker_qid = util_get_queue_info(queue_key, "ngapp_nwucp_worker_queue")) < 0) {
		ERRLOG(LLE, FL, "%s() fatal! queue_id_info.ngapp_nwucp_worker_queue not exist!\n", __func__);
		return (-1);
	}
	config_lookup_int(&MAIN_CTX->CFG, "distr_info.eap5g_nwucp_worker_queue", &queue_key);
	if ((MAIN_CTX->QID_INFO.eap5g_nwucp_worker_qid = util_get_queue_info(queue_key, "eap5g_nwucp_worker_queue")) < 0) {
		ERRLOG(LLE, FL, "%s() fatal! queue_id_info.eap5g_nwucp_worker_queue not exist!\n", __func__);
		return (-1);
	}

	return (0);
}

int create_tcp_server(main_ctx_t *MAIN_CTX)
{
	tcp_ctx_t *tcp_server = &MAIN_CTX->tcp_server;

	if (!config_lookup_string(&MAIN_CTX->CFG, "process_config.tcp_listen_addr", &tcp_server->tcp_listen_addr)) {
		return (-1);
	}
	if (!config_lookup_int(&MAIN_CTX->CFG, "process_config.tcp_listen_port", &tcp_server->tcp_listen_port)) {
		return (-1);
	}
	tcp_server->listen_addr.sin_family = AF_INET;
	tcp_server->listen_addr.sin_addr.s_addr = inet_addr(tcp_server->tcp_listen_addr);
	tcp_server->listen_addr.sin_port = htons(tcp_server->tcp_listen_port);
	tcp_server->listener = evconnlistener_new_bind(MAIN_CTX->evbase_main, listener_cb, NULL,
			LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE|LEV_OPT_THREADSAFE,
			16, (struct sockaddr*)&tcp_server->listen_addr, sizeof(tcp_server->listen_addr));

	if (tcp_server->listener == NULL) {
		ERRLOG(LLE, FL, "%s() fail to create tcp_server (any:%d)!\n", __func__, tcp_server->tcp_listen_port);
		return (-1);
	}

	return (0);
}

int create_worker_thread(worker_thread_t *WORKER, const char *prefix, main_ctx_t *MAIN_CTX)
{
    if (WORKER->worker_num <= 0 || WORKER->worker_num > MAX_WORKER_NUM) {
        ERRLOG(LLE, FL, "%s() fatal! [%s] worker_num=(%d/max=%d)!\n", __func__, prefix, WORKER->worker_num, MAX_WORKER_NUM);
        return (-1);
    }

    WORKER->workers = calloc(1, sizeof(worker_ctx_t) * WORKER->worker_num);

    for (int i = 0; i < WORKER->worker_num; i++) {
        worker_ctx_t *worker_ctx = &WORKER->workers[i];
        worker_ctx->thread_index = i;
        sprintf(worker_ctx->thread_name, "%s_%02d", prefix, i);

        if (pthread_create(&worker_ctx->pthread_id, NULL, io_worker_thread, worker_ctx)) {
            ERRLOG(LLE, FL, "%s() fatal! fail to create thread=(%s)\n", __func__, worker_ctx->thread_name);
            return (-1);
        }

        pthread_setname_np(worker_ctx->pthread_id, worker_ctx->thread_name);
    }

    return 0;
}

int initialize(main_ctx_t *MAIN_CTX)
{
	//
	sprintf(mySysName, "%.15s", getenv("MY_SYS_NAME"));
	sprintf(myAppName, "%.15s", "NWUCP");
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
	if (msgtrclib_init() < 0) {
		ERRLOG(LLE, FL, "[%s.%d] msgtrclib_init() Error\n", FL);
		return -1;
	}
	if (pthread_mutex_init(&MAIN_CTX->mutex_for_ovldctrl, NULL) != 0) {
		ERRLOG(LLE, FL, "[%s.%d] pthread_mutex_init() Error\n", FL);
		return -1;
	}
	if (ovldlib_init(myAppName) < 0) {
		ERRLOG(LLE, FL, "[%s.%d] ovldlib_init() Error\n", FL);
		return -1;
	}
	if (initMmc() < 0) {
		ERRLOG(LLE, FL, "[%s.%d] initMmc() Error\n", FL);
		return -1;
	}

	if ((load_config(&MAIN_CTX->CFG)	< 0) || 
		(create_n3iwf_profile(MAIN_CTX) < 0) ||
		(create_amf_list(MAIN_CTX)		< 0) ||
		(create_ue_list(MAIN_CTX)		< 0) || 
		(create_qid_info(MAIN_CTX)		< 0)) {
		return (-1);
	}

    /* io worker thread create */
    config_lookup_int(&MAIN_CTX->CFG, "process_config.io_worker_num", &MAIN_CTX->IO_WORKERS.worker_num);
    if (create_worker_thread(&MAIN_CTX->IO_WORKERS, "io_worker", MAIN_CTX) < 0) {
        return (-1);
    }

	if (create_tcp_server(MAIN_CTX)	< 0) {
		return (-1);
	}

	/* check ~/data, if sctpc restart to some action */
	start_watching_dir(MAIN_CTX);

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
    GeneralQMsgType *rcv_msg = (GeneralQMsgType *)recv_buffer;
    int recv_size = 0;

    while ((recv_size = msgrcv(MAIN_CTX->QID_INFO.main_qid, rcv_msg, sizeof(recv_buffer), 0, IPC_NOWAIT)) >= 0) {
        switch(rcv_msg->mtype) {
            case MTYPE_SETPRINT:
                continue;
            case MTYPE_MMC_REQUEST:
				handleMMCReq((IxpcQMsgType *)rcv_msg->body);
                continue;
            case MTYPE_STATISTICS_REQUEST:
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
		ERRLOG(LLE, FL, "%s fail to initialize()!\n", __func__);
		exit(0);
	}

    struct timeval one_sec = {1, 0};
    struct event *ev_tick = event_new(MAIN_CTX->evbase_main, -1, EV_PERSIST, main_tick, NULL);
    event_add(ev_tick, &one_sec);

    struct timeval one_u_sec = {0, 1};
	struct event *ev_msgq_read = event_new(MAIN_CTX->evbase_main, -1, EV_PERSIST, main_msgq_read_callback, MAIN_CTX);
	event_add(ev_msgq_read, &one_u_sec);
    struct event *ev_msg_rcv_from_ngapp = event_new(MAIN_CTX->evbase_main, -1, EV_PERSIST, msg_rcv_from_ngapp, "main");
    event_add(ev_msg_rcv_from_ngapp, &one_u_sec);
    struct event *ev_msg_rcv_from_eap5g = event_new(MAIN_CTX->evbase_main, -1, EV_PERSIST, msg_rcv_from_eap5g, "main");
    event_add(ev_msg_rcv_from_eap5g, &one_u_sec);

	event_base_loop(MAIN_CTX->evbase_main, EVLOOP_NO_EXIT_ON_EMPTY);

	return (0);
}
