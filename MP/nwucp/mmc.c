#include <nwucp.h>

extern main_ctx_t *MAIN_CTX;
extern __thread worker_ctx_t *WORKER_CTX;

int numMmcHdlr;

MmcHdlrVector   mmcHdlrVector[4096] =
{
    { "dis-amf-status",     dis_amf_status},
    { "dis-ue-status",     	dis_ue_status},

    {"", NULL}
};

int initMmc()
{
    int i;
    for(i = 0; i < 4096; i++) {
        if(mmcHdlrVector[i].func == NULL)
            break;
    }
    numMmcHdlr = i;

    qsort ( (void*)mmcHdlrVector,
                   numMmcHdlr,
                   sizeof(MmcHdlrVector),
                   mmcHdlrVector_qsortCmp );
    return 1;
}

void handleMMCReq (IxpcQMsgType *rxIxpcMsg)
{
    MMLReqMsgType       *mmlReqMsg;
    MmcHdlrVector       *mmcHdlr;

    mmlReqMsg = (MMLReqMsgType*)rxIxpcMsg->body;

    if ((mmcHdlr = (MmcHdlrVector*) bsearch (
                    mmlReqMsg->head.cmdName,
                    mmcHdlrVector,
                    numMmcHdlr,
                    sizeof(MmcHdlrVector),
                    mmcHdlrVector_bsrchCmp)) == NULL) {
        logPrint(ELI, FL, "handleMMCReq() received unknown mml_cmd(%s)\n", mmlReqMsg->head.cmdName);
        return ;
    }
    logPrint(LL3, FL, "handleMMCReq() received mml_cmd(%s)\n", mmlReqMsg->head.cmdName);

    (int)(*(mmcHdlr->func)) (rxIxpcMsg);

    return;
}

int mmcHdlrVector_qsortCmp (const void *a, const void *b)
{
    return (strcasecmp (((MmcHdlrVector*)a)->cmdName, ((MmcHdlrVector*)b)->cmdName));
}

int mmcHdlrVector_bsrchCmp (const void *a, const void *b)
{
    return (strcasecmp ((char*)a, ((MmcHdlrVector*)b)->cmdName));
}

int dis_amf_status(IxpcQMsgType *rxIxpcMsg)
{
    char    mmlBuf[8109], *p = mmlBuf;

	int		amf_num = link_node_length(&MAIN_CTX->amf_list);

    p += mmclib_printSuccessHeader(p);

	ft_table_t *table = ft_create_table();
	ft_set_border_style(table, FT_PLAIN_STYLE);
	ft_add_separator(table);
	ft_write_ln(table, "[config]", "[ngap_info]");
	ft_add_separator(table);

	for (int i = 0; i < amf_num; i++) {
		amf_ctx_t *amf_ctx = link_node_get_nth_data(&MAIN_CTX->amf_list, i);
		json_object *amf_profile = amf_ctx->js_amf_data;
		if (amf_profile == NULL) { 
			continue;
		}
		ft_table_t *ngap_table = ft_create_table();
		ft_set_border_style(ngap_table, FT_PLAIN_STYLE);
		ft_write_ln(ngap_table, "field", "value");
		ft_add_separator(ngap_table);

		const char *amf_name = ngap_get_amf_name(amf_profile);
		int64_t relative_amf_capacity = ngap_get_relative_amf_capacity(amf_profile);
		json_object *served_guami_list = ngap_get_served_gaumi_list(amf_profile);
		json_object *plmn_support_list = ngap_get_plmn_support_list(amf_profile);

		ft_printf_ln(ngap_table, "amf_name|%s", amf_name);
		ft_printf_ln(ngap_table, "amf_capacity|%ld", relative_amf_capacity);
		ft_printf_ln(ngap_table, "served_guami_list|%s", JS_PRINT_PRETTY(served_guami_list));
		ft_printf_ln(ngap_table, "plmn_support_list|%s", JS_PRINT_PRETTY(plmn_support_list));

		ft_printf_ln(table, "hostname  (%s)\nstatus    (%s)\nresp time (%s)|%s",
				amf_ctx->hostname, 
				amf_ctx->ng_setup_status == AMF_RESP_SUCCESS ? "Recv Success" :
				amf_ctx->ng_setup_status == AMF_RESP_UNSUCCESS ? "Recv Failure" : "No Response",
				amf_ctx->amf_regi_tm,
				ft_to_string(ngap_table));

		ft_destroy_table(ngap_table);
		ft_add_separator(table);
	}
	p += sprintf(p, ft_to_string(table));
	ft_destroy_table(table);

    p += sprintf(p, " TOTAL = %d\n", amf_num);
    mmclib_sendMmcResult(rxIxpcMsg, mmlBuf, RES_SUCCESS, FLAG_COMPLETE, 0, 0, 0);

    return 0;
}

int dis_ue_status(IxpcQMsgType *rxIxpcMsg)
{
	char mmlBuf[4096] = {0,};
	char value[MAX_TRC_LEN+1] = {0,};

	if (mmclib_getMmlPara_STR(rxIxpcMsg, "UE_ID", value) < 0) {
		mmclib_printFailureHeader(mmlBuf, "MANDATORY PARAMETER(UE_ID) MISSING");
		mmclib_sendMmcResult(rxIxpcMsg, mmlBuf, RES_FAIL, FLAG_COMPLETE, 0, 0, 0);
		return -1;
	}

	mmc_temp_mem_t *mmc_temp = malloc(sizeof(mmc_temp_mem_t));
	sprintf(mmc_temp->ue_id, "%s", value);
	memcpy(&mmc_temp->head, &rxIxpcMsg->head, sizeof(IxpcQMsgHeadType));
	memcpy(&mmc_temp->body, &rxIxpcMsg->body, sizeof(MMLReqMsgType));

	worker_ctx_t *worker_ctx = worker_ctx_get_by_index(0);

	/* find from worker (index:0) */
	event_base_once(worker_ctx->evbase_thrd, -1, EV_TIMEOUT, dis_ue_status_cb, mmc_temp, NULL);

	return 0;
}

int dis_ue_fill_print(ue_ctx_t *ue_ctx, char *buffer)
{
	int write_bytes = 0;

	ft_table_t *table = ft_create_table();
	ft_set_border_style(table, FT_PLAIN_STYLE);
	ft_add_separator(table);
	ft_write_ln(table, "[info]", "[field]", "[value]");
	ft_add_separator(table);

	ike_tag_t *ike_tag = &ue_ctx->ike_tag;
	ft_printf_ln(table, "IKE_INFO|ue_from_addr|%s", ike_tag->ue_from_addr);
	ft_printf_ln(table, "|ue_from_port|%d", ike_tag->ue_from_port);
	ft_printf_ln(table, "|up_from_addr|%s", ike_tag->up_from_addr);
	ft_printf_ln(table, "|up_from_port|%d", ike_tag->up_from_port);
	ft_add_separator(table);

	amf_tag_t *amf_tag = &ue_ctx->amf_tag;
	ft_printf_ln(table, "AMF_INFO|amf_host|%s", amf_tag->amf_host);
	ft_printf_ln(table, "|amf_ue_id|%ld", amf_tag->amf_ue_id);
	ft_printf_ln(table, "|amf_ran_id|%ld", amf_tag->ran_ue_id);
	ft_add_separator(table);

	ctx_info_t *ctx_info = &ue_ctx->ctx_info;
	ft_printf_ln(table, "CTX_INFO|ue_id|%s", ctx_info->ue_id);
	ft_printf_ln(table, "|up_id|%d", ctx_info->up_id);
	ft_printf_ln(table, "|cp_id|%d", ctx_info->cp_id);
	ft_printf_ln(table, "|last_state|%s", ue_ctx->state);
	ft_printf_ln(table, "|assigned_ip|%s", ue_ctx->ip_addr);
	ft_printf_ln(table, "|ipsec_state|%s", ue_ctx->ipsec_sa_created > 0 ? "Created" : "Not Created");
	ft_add_separator(table);

	sock_ctx_t *sock_ctx = ue_ctx->sock_ctx;
	ft_printf_ln(table, "TCP_INFO|tcp_state|%s", sock_ctx != NULL ? "Connected" : "Not Connected");
	if (sock_ctx != NULL) {
		ft_printf_ln(table, "|client_ip|%s", sock_ctx->client_ipaddr);
		ft_printf_ln(table, "|client_port|%d", sock_ctx->client_port);
	}
	ft_add_separator(table);

	json_object *js_ue_regi_data = ue_ctx->js_ue_regi_data;
	ft_printf_ln(table, "NGAP_INFO|UE Profile|%s", js_ue_regi_data != NULL ? "Received" : "Not Received");
	if (js_ue_regi_data) {
		ft_printf_ln(table, "|AMF-UE-NGAP-ID|%ld", ngap_get_amf_ue_ngap_id(js_ue_regi_data));
		ft_printf_ln(table, "|RAN-UE-NGAP-ID|%ld", ngap_get_ran_ue_ngap_id(js_ue_regi_data));
		ft_printf_ln(table, "|GUAMI|%s", JS_PRINT_PRETTY(ngap_get_guami(js_ue_regi_data)));
		ft_printf_ln(table, "|AllowedNSSAI|%s", JS_PRINT_PRETTY(ngap_get_allowed_nssai(js_ue_regi_data)));
		ft_printf_ln(table, "|UESecurityCapabilities|%s", JS_PRINT_PRETTY(ngap_get_ue_security_capabilities(js_ue_regi_data)));
		ft_printf_ln(table, "|SecurityKey|%s", ngap_get_security_key(js_ue_regi_data));
		ft_printf_ln(table, "|MaskedIMEISV|%s", ngap_get_masked_imeisv(js_ue_regi_data));
	}
	ft_add_separator(table);

	write_bytes = sprintf(buffer, "%s", ft_to_string(table));
	ft_destroy_table(table);

	return write_bytes;
}

#define	MAKE_MMC_IXPC(a,b) { memcpy(&a->head, &b->head, sizeof(b->head)); memcpy(&a->body, &b->body, sizeof(b->body)); }
void dis_ue_status_cb(int conn_fd, short events, void *data)
{
	IxpcQMsgType	ixpc_buffer, *rxIxpcMsg = &ixpc_buffer;
    char			mmlBuf[8109], *p = mmlBuf;
	mmc_temp_mem_t *mmc_temp = (mmc_temp_mem_t *)data;
	ue_ctx_t	   *ue_ctx_find = NULL;

	/* find ue ctx */
	for (int index = 0; index < MAIN_CTX->ue_info.ue_num; index++) {
		if ((index % MAIN_CTX->IO_WORKERS.worker_num) != WORKER_CTX->thread_index) {
			continue;
		}
		ue_ctx_t *ue_ctx = &MAIN_CTX->ue_info.ue_ctx[index];
		if (ue_ctx->occupied == 0) {
			continue;
		}
		if (!strcmp(ue_ctx->ctx_info.ue_id, mmc_temp->ue_id)) {
			ue_ctx_find = ue_ctx;
			break;
		}
	}

	/* OK find ue */
	if (ue_ctx_find != NULL) {
		MAKE_MMC_IXPC(rxIxpcMsg, mmc_temp);
		p += mmclib_printSuccessHeader(p);
		p += dis_ue_fill_print(ue_ctx_find, p);
		mmclib_sendMmcResult(rxIxpcMsg, mmlBuf, RES_SUCCESS, FLAG_COMPLETE, 0, 0, 0);
		free(mmc_temp);
		return;
	}

	/* NOK but try next */
	if ((WORKER_CTX->thread_index + 1) < MAIN_CTX->IO_WORKERS.worker_num) {
		worker_ctx_t *worker_ctx = worker_ctx_get_by_index(WORKER_CTX->thread_index + 1);
		event_base_once(worker_ctx->evbase_thrd, -1, EV_TIMEOUT, dis_ue_status_cb, mmc_temp, NULL);
		return;
	}

	/* FAIL reach */
	MAKE_MMC_IXPC(rxIxpcMsg, mmc_temp);
	p += mmclib_printFailureHeader(p, "FAIL TO FIND UE");
	mmclib_sendMmcResult(rxIxpcMsg, mmlBuf, RES_SUCCESS, FLAG_COMPLETE, 0, 0, 0);
	free(mmc_temp);
	return;
}
