#include <nwucp.h>

main_ctx_t main_ctx, *MAIN_CTX = &main_ctx;

int load_config(config_t *CFG)
{
	/* load config */
	config_init(CFG);

    if (!config_read_file(CFG, "./nwucp.cfg")) {
        fprintf(stderr, "cfg err!\n");
        fprintf(stderr, "%s() fatal! fail to load cfg file=(%s) line:text(%d/%s)!\n",
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

    fprintf(stderr, "%s\n", JS_PRINT_PRETTY(MAIN_CTX->js_ng_setup_request));

	json_object_put(json_cfg);

	return (0);
}

int create_qid_info(main_ctx_t *MAIN_CTX)
{
	int queue_key = 0;
	config_lookup_int(&MAIN_CTX->CFG, "queue_id_info.ngapp_nwucp_queue", &queue_key);
	if ((MAIN_CTX->QID_INFO.ngapp_nwucp_qid = util_get_queue_info(queue_key, "ngapp_nwucp_queue")) < 0) {
		return (-1);
	}
	config_lookup_int(&MAIN_CTX->CFG, "queue_id_info.nwucp_ngapp_queue", &queue_key);
	if ((MAIN_CTX->QID_INFO.nwucp_ngapp_qid = util_get_queue_info(queue_key, "nwucp_ngapp_queue")) < 0) {
		return (-1);
	}
	config_lookup_int(&MAIN_CTX->CFG, "queue_id_info.eap5g_nwucp_queue", &queue_key);
	if ((MAIN_CTX->QID_INFO.eap5g_nwucp_qid = util_get_queue_info(queue_key, "eap5g_nwucp_queue")) < 0) {
		return (-1);
	}
	config_lookup_int(&MAIN_CTX->CFG, "queue_id_info.nwucp_eap5g_queue", &queue_key);
	if ((MAIN_CTX->QID_INFO.nwucp_eap5g_qid = util_get_queue_info(queue_key, "nwucp_eap5g_queue")) < 0) {
		return (-1);
	}
	config_lookup_int(&MAIN_CTX->CFG, "distr_info.ngapp_nwucp_worker_queue", &queue_key);
	if ((MAIN_CTX->QID_INFO.ngapp_nwucp_worker_qid = util_get_queue_info(queue_key, "ngapp_nwucp_worker_queue")) < 0) {
		return (-1);
	}
	config_lookup_int(&MAIN_CTX->CFG, "distr_info.eap5g_nwucp_worker_queue", &queue_key);
	if ((MAIN_CTX->QID_INFO.eap5g_nwucp_worker_qid = util_get_queue_info(queue_key, "eap5g_nwucp_worker_queue")) < 0) {
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
		fprintf(stderr, "%s() fail to create tcp_server (any:%d)!\n", __func__, tcp_server->tcp_listen_port);
		return (-1);
	}

	return (0);
}

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

        pthread_setname_np(worker_ctx->pthread_id, worker_ctx->thread_name);
        fprintf(stderr, "setname=[%s]\n", worker_ctx->thread_name);
    }

    return 0;
}

int initialize(main_ctx_t *MAIN_CTX)
{
	if ((load_config(&MAIN_CTX->CFG)	< 0) || 
		(create_n3iwf_profile(MAIN_CTX) < 0) ||
		(create_amf_list(MAIN_CTX)		< 0) ||
		(create_ue_list(MAIN_CTX)		< 0) || 
		(create_qid_info(MAIN_CTX)		< 0) ||
		(create_tcp_server(MAIN_CTX)	< 0)) {
		return (-1);
	}

    /* io worker thread create */
    config_lookup_int(&MAIN_CTX->CFG, "process_config.io_worker_num", &MAIN_CTX->IO_WORKERS.worker_num);
    if (create_worker_thread(&MAIN_CTX->IO_WORKERS, "io_worker", MAIN_CTX) < 0) {
        return (-1);
    }

	return (0);
}

void main_tick(int conn_fd, short events, void *data)
{
	//fprintf(stderr, "%s called!\n", __func__);
}

int main()
{
	evthread_use_pthreads();

    /* create main */
    MAIN_CTX->evbase_main = event_base_new();

	if (initialize(MAIN_CTX) < 0) {
		fprintf(stderr, "%s fail to initialize()!\n", __func__);
		exit(0);
	}

    struct timeval one_sec = {1, 0};
    struct event *ev_tick = event_new(MAIN_CTX->evbase_main, -1, EV_PERSIST, main_tick, NULL);
    event_add(ev_tick, &one_sec);

    struct timeval u_sec = {0, 1};
    struct event *ev_msg_rcv_from_ngapp = event_new(MAIN_CTX->evbase_main, -1, EV_PERSIST, msg_rcv_from_ngapp, "main");
    event_add(ev_msg_rcv_from_ngapp, &u_sec);
    struct event *ev_msg_rcv_from_eap5g = event_new(MAIN_CTX->evbase_main, -1, EV_PERSIST, msg_rcv_from_eap5g, "main");
    event_add(ev_msg_rcv_from_eap5g, &u_sec);

	event_base_loop(MAIN_CTX->evbase_main, EVLOOP_NO_EXIT_ON_EMPTY);

	return (0);
}
