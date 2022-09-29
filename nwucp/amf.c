#include <nwucp.h>
extern main_ctx_t *MAIN_CTX;

int create_amf_list(main_ctx_t *MAIN_CTX)
{
	config_setting_t *cf_amf_list = config_lookup(&MAIN_CTX->CFG, "n3iwf_info.amf_list");
	for (int i = 0; i < config_setting_length(cf_amf_list); i++) {
		config_setting_t *cf_amf = config_setting_get_elem(cf_amf_list, i);
		const char *hostname = config_setting_get_string(cf_amf);
		fprintf(stderr, "%s() create amf [%s] context\n", __func__, hostname);

		/* make amf node ctx */
		link_node *amf_node = link_node_assign_key_order(&MAIN_CTX->amf_list, hostname, sizeof(amf_ctx_t));
		amf_ctx_t *amf_ctx = amf_node->data;
		sprintf(amf_ctx->hostname, "%s", hostname);

		/* start ng_setup */
		amf_regi_start(MAIN_CTX, amf_ctx);
	}

	return (0);
}

void amf_regi(int conn_fd, short events, void *data)
{
	amf_ctx_t *amf_ctx = (amf_ctx_t *)data;

	if (amf_ctx->ng_setup_status > 0) {
		return;
	}

	fprintf(stderr, "%s() try [NGSetupRequest] to amf [%s]\n", __func__, amf_ctx->hostname);

	ngap_send_json(amf_ctx->hostname, MAIN_CTX->js_ng_setup_request);
}

void amf_regi_start(main_ctx_t *MAIN_CTX, amf_ctx_t *amf_ctx)
{
	int amf_regi_interval = 0;
	if (config_lookup_int(&MAIN_CTX->CFG, "n3iwf_info.amf_regi_interval", &amf_regi_interval) < 0) {
		fprintf(stderr, "%s() fail to get [n3iwf_info.amf_regi_interval] in .cfg!\n", __func__);
		exit(0);
	}

	fprintf(stderr, "%s() amf [%s] interval (%d) sec\n", __func__, amf_ctx->hostname, amf_regi_interval);

	struct timeval regi_tm = { amf_regi_interval, 0 };
	amf_ctx->ev_amf_regi = event_new(MAIN_CTX->evbase_main, -1, EV_PERSIST, amf_regi, amf_ctx);
	event_add(amf_ctx->ev_amf_regi, &regi_tm);
}

void amf_ctx_unset(amf_ctx_t *amf_ctx)
{
	if (amf_ctx->ev_amf_regi) {
		event_free(amf_ctx->ev_amf_regi);
		amf_ctx->ev_amf_regi = NULL;
	}
	if (amf_ctx->js_amf_data != NULL) {
		json_object_put(amf_ctx->js_amf_data);
		amf_ctx->js_amf_data = NULL;
	}
}

void amf_regi_res_handle(sctp_tag_t *sctp_tag, bool success, json_object *js_ngap_pdu)
{
	amf_ctx_t *amf_ctx = link_node_get_data_by_key(&MAIN_CTX->amf_list, sctp_tag->hostname);

	if (amf_ctx == NULL) {
		fprintf(stderr, "%s() can't find amf [%s]!\n", __func__, sctp_tag->hostname);
		return;
	}
	if (success != true) {
		fprintf(stderr, "%s() recv NOK response from amf [%s], will retry!\n", __func__, sctp_tag->hostname);
		return;
	}

	fprintf(stderr, "%s() recv OK response from amf [%s]\n", __func__, sctp_tag->hostname);

	amf_ctx_unset(amf_ctx);

	json_object_deep_copy(js_ngap_pdu, &amf_ctx->js_amf_data, NULL);
}
