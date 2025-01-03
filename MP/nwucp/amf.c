#include <nwucp.h>
extern main_ctx_t *MAIN_CTX;

int create_amf_list(main_ctx_t *MAIN_CTX)
{
	config_setting_t *cf_amf_list = config_lookup(&MAIN_CTX->CFG, "n3iwf_info.amf_list");
	for (int i = 0; i < config_setting_length(cf_amf_list); i++) {
		config_setting_t *cf_amf = config_setting_get_elem(cf_amf_list, i);
		const char *hostname = config_setting_get_string(cf_amf);
		ERRLOG(LLE, FL, "%s() create amf [%s] context\n", __func__, hostname);

		/* make amf node ctx */
		link_node *amf_node = link_node_assign_key_order(&MAIN_CTX->amf_list, hostname, sizeof(amf_ctx_t));
		amf_ctx_t *amf_ctx = amf_node->data;
		sprintf(amf_ctx->hostname, "%s", hostname);

		/* start ng_setup */
		amf_regi_start(MAIN_CTX, amf_ctx);
	}

	return (0);
}

void remove_amf_list(main_ctx_t *MAIN_CTX)
{
	link_node_delete_all(&MAIN_CTX->amf_list, amf_ctx_unset);
}

void amf_regi(int conn_fd, short events, void *data)
{
	amf_ctx_t *amf_ctx = (amf_ctx_t *)data;

	amf_ctx->ng_setup_status = AMF_NO_RESP;

	ERRLOG(LLE, FL, "%s() try [NGSetupRequest] to amf [%s]\n", __func__, amf_ctx->hostname);

	ngap_send_json(amf_ctx->hostname, NULL, MAIN_CTX->js_ng_setup_request);
}

void amf_regi_start(main_ctx_t *MAIN_CTX, amf_ctx_t *amf_ctx)
{
	int amf_regi_interval = 0;
	if (config_lookup_int(&MAIN_CTX->CFG, "n3iwf_info.amf_regi_interval", &amf_regi_interval) < 0) {
		ERRLOG(LLE, FL, "%s() fail to get [n3iwf_info.amf_regi_interval] in .cfg!\n", __func__);
		exit(0);
	}

	ERRLOG(LLE, FL, "%s() amf [%s] interval (%d) sec\n", __func__, amf_ctx->hostname, amf_regi_interval);

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

int amf_regi_res_handle(ngap_msg_t *ngap_msg, bool success, json_object *js_ngap_pdu)
{
	sctp_tag_t *sctp_tag = &ngap_msg->sctp_tag;

	amf_ctx_t *amf_ctx = link_node_get_data_by_key(&MAIN_CTX->amf_list, sctp_tag->hostname);

	if (amf_ctx == NULL) {
		ERRLOG(LLE, FL, "%s() can't find amf [%s]!\n", __func__, sctp_tag->hostname);
		goto ARRH_ERR;
	} else {
		time_t curr_tm = time(NULL);
		sprintf(amf_ctx->amf_regi_tm, "%.19s", ctime(&curr_tm));
	}

	if (success != true) {
		ERRLOG(LLE, FL, "%s() recv NOK response from amf [%s], will retry!\n", __func__, sctp_tag->hostname);
		goto ARRH_ERR;
	}

	ERRLOG(LLE, FL, "%s() recv OK response from amf [%s]\n", __func__, sctp_tag->hostname);

	amf_ctx_unset(amf_ctx);

	amf_ctx->ng_setup_status = AMF_RESP_SUCCESS;

	json_object_deep_copy(js_ngap_pdu, &amf_ctx->js_amf_data, NULL);

	return 0;

ARRH_ERR:
	if (amf_ctx != NULL) {
		amf_ctx->ng_setup_status = AMF_RESP_UNSUCCESS;
	}

	return -1;
}

int amf_status_ind_handle(ngap_msg_t *ngap_msg, json_object *js_ngap_pdu)
{
	sctp_tag_t *sctp_tag = &ngap_msg->sctp_tag;

	amf_ctx_t *amf_ctx = link_node_get_data_by_key(&MAIN_CTX->amf_list, sctp_tag->hostname);

	if (amf_ctx == NULL) {
		ERRLOG(LLE, FL, "%s() can't find amf [%s]!\n", __func__, sctp_tag->hostname);
		return -1;
	}

	json_object *js_unavailable_guami_list = ngap_get_unavailable_guami_list(js_ngap_pdu);

	if (js_unavailable_guami_list != NULL) {
		ERRLOG(LLE, FL, "%s() check! unavailable guami list (IEs:120)\n%s\n", __func__, JS_PRINT_PRETTY(js_unavailable_guami_list));

		/* re-start ng_setup */
		amf_ctx_unset(amf_ctx);
		amf_regi_start(MAIN_CTX, amf_ctx);
	}

	return 0;
}

