#include <nwucp.h>
extern main_ctx_t *MAIN_CTX;

int create_amf_list(main_ctx_t *MAIN_CTX)
{
	config_setting_t *cf_amf_list = config_lookup(&MAIN_CTX->CFG, "n3iwf_info.amf_list");
	for (int i = 0; i < config_setting_length(cf_amf_list); i++) {
		config_setting_t *cf_amf = config_setting_get_elem(cf_amf_list, i);
		const char *hostname = config_setting_get_string(cf_amf);
		fprintf(stderr, "%s check amf=[%s]\n", __func__, hostname);

		/* make amf node ctx */
		link_node *amf_node = link_node_assign_key_order(&MAIN_CTX->amf_list, hostname, sizeof(amf_ctx_t));
		amf_ctx_t *amf_ctx = amf_node->data;
		sprintf(amf_ctx->hostname, "%s", hostname);

		/* start ng_setup */
		amf_regi_start(MAIN_CTX, amf_ctx);
	}

	return (0);
}


void amf_regi(evutil_socket_t fd, short events, void *data)
{
	amf_ctx_t *amf_ctx = (amf_ctx_t *)data;

	fprintf(stderr, "{dbg} %s called for amf=[%s]\n", __func__, amf_ctx->hostname);

	if (amf_ctx->ng_setup_status > 0) {
		return;
	}

	ngap_msg_t msg = { .mtype = 1, }, *ngap_msg = &msg;
	sprintf(ngap_msg->sctp_tag.hostname, "%s", amf_ctx->hostname);
	ngap_msg->msg_size = sprintf(ngap_msg->msg_body, "%s", JS_PRINT_COMPACT(MAIN_CTX->js_ng_setup_request));

	int res = msgsnd(MAIN_CTX->QID_INFO.my_send_queue, ngap_msg, NGAP_MSG_SIZE(ngap_msg), IPC_NOWAIT);
	fprintf(stderr, "{dbg} %s msgsnd res=(%d) size=(%ld) (%d:%s) (%ld)\n", __func__, res, ngap_msg->msg_size, errno, strerror(errno), NGAP_MSG_SIZE(ngap_msg));
}

void amf_regi_start(main_ctx_t *MAIN_CTX, amf_ctx_t *amf_ctx)
{
	int amf_regi_interval = 0;
	if (config_lookup_int(&MAIN_CTX->CFG, "n3iwf_info.amf_regi_interval", &amf_regi_interval) < 0) {
		fprintf(stderr, "{dbg} fail to get [n3iwf_info.amf_regi_interval] in .cfg!\n");
		exit(0);
	} else {
		fprintf(stderr, "%s amf=[%s] interval=(%d)\n", __func__, amf_ctx->hostname, amf_regi_interval);
	}

	struct timeval regi_tm = { amf_regi_interval, 0 };
	amf_ctx->ev_amf_regi = event_new(MAIN_CTX->evbase_main, -1, EV_PERSIST, amf_regi, amf_ctx);
	event_add(amf_ctx->ev_amf_regi, &regi_tm);
}
