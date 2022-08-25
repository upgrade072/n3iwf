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

	return (0);
}

int initialize(main_ctx_t *MAIN_CTX)
{
	if ((load_config(&MAIN_CTX->CFG) < 0) || 
		(create_n3iwf_profile(MAIN_CTX) < 0) ||
		(create_amf_list(MAIN_CTX) < 0) ||
		(create_ue_list(MAIN_CTX) < 0) || 
		(create_qid_info(MAIN_CTX) < 0)) {
		return (-1);
	}

	return (0);
}

void main_tick(evutil_socket_t fd, short events, void *data)
{
	//fprintf(stderr, "%s called!\n", __func__);
}

void handle_ngap_msg(ngap_msg_t *ngap_msg)
{
	sctp_tag_t *sctp_tag = &ngap_msg->sctp_tag;
	fprintf(stderr, "{dbg} from sctp [h:%s a:%d s:%d p:%d]\n", 
			sctp_tag->hostname, sctp_tag->assoc_id, sctp_tag->stream_id, sctp_tag->ppid);

	json_object *js_ngap_pdu = json_tokener_parse(ngap_msg->msg_body);

	key_list_t *key_list = &ngap_msg->ngap_tag.key_list;
	memset(key_list, 0x00, sizeof(key_list_t));

	int proc_code = json_object_get_int(search_json_object_ex(js_ngap_pdu, "/*/procedureCode", key_list));
	int success = !strcmp(key_list->key_val[0], "successfulOutcome") ? true : false;

	switch (proc_code)  
	{
	/* CAUTION! release js_ngap_pdu in func() */
		case NGSetupResponse:
			return amf_regi_res_handle(sctp_tag, success, js_ngap_pdu);
		default:
			/* we can't handle, just discard */
			if (js_ngap_pdu != NULL) {
				json_object_put(js_ngap_pdu);
			}
			break;
	}
}

void main_msg_rcv(evutil_socket_t fd, short events, void *data)
{
	ngap_msg_t msg = {0,}, *ngap_msg = &msg;

	while (msgrcv(MAIN_CTX->QID_INFO.ngapp_nwucp_qid, ngap_msg, sizeof(ngap_msg_t), 0, IPC_NOWAIT) > 0) {
		handle_ngap_msg(ngap_msg);
	}
}

int main()
{
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
    struct event *ev_msg_rcv = event_new(MAIN_CTX->evbase_main, -1, EV_PERSIST, main_msg_rcv, NULL);
    event_add(ev_msg_rcv, &u_sec);

	event_base_loop(MAIN_CTX->evbase_main, EVLOOP_NO_EXIT_ON_EMPTY);

	return (0);
}
