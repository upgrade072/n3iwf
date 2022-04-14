#include <nwucp.h>

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
	}

	return (0);
}
