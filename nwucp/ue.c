#include <nwucp.h>

extern main_ctx_t *MAIN_CTX;

int create_ue_list(main_ctx_t *MAIN_CTX)
{
	config_setting_t *cfg_ue_ip_range = config_lookup(&MAIN_CTX->CFG, "n3iwf_info.ue_ip_range");
	const char *ip_range = config_setting_get_string(cfg_ue_ip_range);

	char ip_start[256] = {0,}, ip_end[256] = {0,};
	int ret = (ipaddr_range_scan(ip_range, ip_start, ip_end));
	if (ret <= 0) {
		fprintf(stderr, "%s fail to get ip range from=[%s]\n", __func__, ip_range);
		return (-1);
	}

	int num_of_addr = ipaddr_range_calc(ip_start, ip_end);
	if (num_of_addr <= 0 || num_of_addr > MAX_NWUCP_UE_NUM) {
		fprintf(stderr, "%s fail cause num_of_addr=[%d] wrong\n", __func__, num_of_addr);
		return (-1);
	}

	MAIN_CTX->ue_info.ue_num = num_of_addr;
	MAIN_CTX->ue_info.cur_index = 0; // for assign start pos
	MAIN_CTX->ue_info.ue_ctx = calloc(1, sizeof(ue_ctx_t) * MAIN_CTX->ue_info.ue_num);

	char *ip_list = strdup(ip_start);
	for (int i = 0; i < MAIN_CTX->ue_info.ue_num; i++) {
		ue_ctx_t *ue_ctx = &MAIN_CTX->ue_info.ue_ctx[i];
		ue_ctx->index = i;
		sprintf(ue_ctx->ip_addr, "%s", ip_list);
		ip_list = ipaddr_increaser(ip_list);
	}

	return (0);
}

ue_ctx_t *get_ue_ctx(main_ctx_t *MAIN_CTX)
{
	for (int i = 0; i < MAIN_CTX->ue_info.ue_num; i++) {
		int index = (MAIN_CTX->ue_info.cur_index + i) % MAIN_CTX->ue_info.ue_num;
		ue_ctx_t *ue_ctx = &MAIN_CTX->ue_info.ue_ctx[index];
		if (ue_ctx->occupied == 0) {
			ue_ctx->occupied = 1;
			MAIN_CTX->ue_info.cur_index = index;
			fprintf(stderr, "{dbg} %s() assign ue ctx (index:%d, ip:%s)\n", __func__, index, ue_ctx->ip_addr);
			return ue_ctx;
		}
	}

	return NULL;
}

worker_ctx_t *get_worker_ctx_by_ue(ue_ctx_t *ue_ctx)
{
	return &MAIN_CTX->IO_WORKERS.workers[ue_ctx->index % MAIN_CTX->IO_WORKERS.worker_num];
}

void ike_auth_assign_ue(ike_msg_t *ike_msg)
{
	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;
	ue_ctx_t *ue_ctx = get_ue_ctx(MAIN_CTX);

	/* TODO IKE INFORM BACKOFF */
	if (ue_ctx == NULL) {
		return;
	}
	memcpy(&ue_ctx->ike_tag, &ike_msg->ike_tag, sizeof(ike_tag_t));
	memcpy(&ue_ctx->ctx_info, &n3iwf_msg->ctx_info, sizeof(ctx_info_t));
	ue_ctx->ctx_info.cp_id = ue_ctx->index;

	worker_ctx_t *worker_ctx = get_worker_ctx_by_ue(ue_ctx);

	event_base_once(worker_ctx->evbase_thrd, -1, EV_TIMEOUT, eap_req_5g_start, ue_ctx, NULL);
}

uint8_t get_eap_id()
{
	srand((unsigned int)time(NULL));
	uint8_t id = rand();
	return id;
}

void ue_ctx_release(int conn_fd, short events, void *data)
{
	// TODO 
	fprintf(stderr, "{dbg TODO} %s() called!\n", __func__);

	ue_ctx_t *ue_ctx = (ue_ctx_t *)data;

	ue_ctx->occupied = 0; // and timer null event del ...
}

void eap_req_5g_start(int conn_fd, short events, void *data)
{
	ue_ctx_t *ue_ctx = (ue_ctx_t *)data;
	worker_ctx_t *worker_ctx = get_worker_ctx_by_ue(ue_ctx);

	eap_relay_t *eap_5g = &ue_ctx->eap_5g;
	memset(eap_5g, 0x00, sizeof(eap_relay_t));

	/* make eap start message at ue_ctx */
	eap_5g->eap_code = EAP_REQUEST;
	eap_5g->eap_id = get_eap_id();
	eap_5g->msg_id = NAS_5GS_START;

	/* make IKE_AUTH_RES */
	ike_msg_t msg = { .mtype = 1 }, *ike_msg = &msg;
	// ue_id part
	memcpy(&ike_msg->ike_tag, &ue_ctx->ike_tag, sizeof(ike_tag_t));
	// n3iwf_msg part
	n3iwf_msg_t *n3iwf_msg = &ike_msg->n3iwf_msg;
	n3iwf_msg->msg_code = N3_IKE_AUTH_RES;
	n3iwf_msg->res_code = N3_EAP_REQUEST;
	memcpy(&n3iwf_msg->ctx_info, &ue_ctx->ctx_info, sizeof(ctx_info_t));
	// eap_5g_part
	memcpy(&ike_msg->eap_5g, eap_5g, sizeof(eap_relay_t));

	/* start timer */
	if (ue_ctx->ev_timer != NULL) {
		event_del(ue_ctx->ev_timer);
		ue_ctx->ev_timer = NULL;
	}
	struct timeval tmout_sec = { N3_EXPIRE_TM_SEC, 0 };
	ue_ctx->state = "eap_request";
	ue_ctx->ev_timer = event_new(worker_ctx->evbase_thrd, -1, EV_TIMEOUT, ue_ctx_release, ue_ctx);
	event_add(ue_ctx->ev_timer, &tmout_sec);

	/* send message to EAP5G */
	int res = msgsnd(MAIN_CTX->QID_INFO.nwucp_eap5g_qid, ike_msg, IKE_MSG_SIZE, IPC_NOWAIT);
	fprintf(stderr, "{dbg} %s send res=(%d) if err (%d:%s)\n", __func__, res, errno, strerror(errno));
}
