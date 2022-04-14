#include <nwucp.h>

int create_ue_list(main_ctx_t *MAIN_CTX)
{
	config_setting_t *cfg_ip_range = config_lookup(&MAIN_CTX->CFG, "n3iwf_info.ip_range");
	const char *ip_range = config_setting_get_string(cfg_ip_range);

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
		sprintf(ue_ctx->ip_addr, "%s", ip_list);
		ip_list = ipaddr_increaser(ip_list);
	}

	return (0);
}
