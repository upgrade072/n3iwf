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

int initialize(main_ctx_t *MAIN_CTX)
{
	if ((load_config(&MAIN_CTX->CFG) < 0) || 
		(create_n3iwf_profile(MAIN_CTX) < 0) ||
		(create_amf_list(MAIN_CTX) < 0) ||
		(create_ue_list(MAIN_CTX) < 0)) {
		return (-1);
	}

	return (0);
}

void main_tick(evutil_socket_t fd, short events, void *data)
{
	fprintf(stderr, "%s called!\n", __func__);
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

	event_base_loop(MAIN_CTX->evbase_main, EVLOOP_NO_EXIT_ON_EMPTY);

	return (0);
}
