#include <nwucp.h>

extern main_ctx_t *MAIN_CTX;

pdu_ctx_t *create_pdu_ctx(int pdu_sess_id, int state, const char *pdu_nas_pdu)
{
	pdu_ctx_t *pdu_ctx = malloc(sizeof(pdu_ctx_t));
	pdu_ctx->pdu_sess_id = pdu_sess_id;
	pdu_ctx->pdu_state = state;
	pdu_ctx->pdu_nas_pdu = strdup(pdu_nas_pdu);

	return pdu_ctx;
}

int pdu_proc_fill_qos_flow_ids(n3_pdu_sess_t *pdu_sess, json_object *js_pdu_qos_flow_ids /* x MAX_N3_QOS_FLOW_IDS */)
{
	int num_of_qos_flow_ids = 0;

	for (int i = 0; i < json_object_array_length(js_pdu_qos_flow_ids) && num_of_qos_flow_ids < MAX_N3_QOS_FLOW_IDS; i++) {
		/* get item */
		json_object *js_elem = json_object_array_get_idx(js_pdu_qos_flow_ids, i);

		/* check mandatory */
		json_object *js_qos_flow_id = search_json_object(js_elem, "/qosFlowIdentifier");
		if (js_qos_flow_id == NULL) {
			ERRLOG(LLE, FL, "TODO %s() check mandatory fail!\n", __func__);
			continue;
		}

		pdu_sess->qos_flow_id[num_of_qos_flow_ids++] = json_object_get_int(js_qos_flow_id);
	}

	return num_of_qos_flow_ids;
}

int pdu_proc_fill_pdu_sess_release_list(ue_ctx_t *ue_ctx, json_object *js_pdu_sess_list, n3_pdu_sess_t *pdu_sess_list /* x MAX_N3_PDU_NUM */)
{
	int num_of_pdu = 0;

	for (int i = 0; i < json_object_array_length(js_pdu_sess_list) && num_of_pdu < MAX_N3_PDU_NUM; i++) {
		/* get item */
		json_object *js_elem = json_object_array_get_idx(js_pdu_sess_list, i);

		/* check mandatory */
		int pdu_sess_id						= ngap_get_pdu_sess_id(js_elem);
		const char *pdu_sess_rel_cause		= ngap_get_pdu_sess_rel_cause(js_elem);

		if (pdu_sess_id	< 0 || pdu_sess_rel_cause	== NULL) {
			ERRLOG(LLE, FL, "TODO %s() check mandatory fail!\n", __func__);
			continue;
		} else {
			ERRLOG(LLE, FL, "%s() ue [%s] delete pdu (id:%d) cause(%s)\n", __func__, UE_TAG(ue_ctx), pdu_sess_id, pdu_sess_rel_cause);
		}

		n3_pdu_sess_t *pdu_sess = &pdu_sess_list[num_of_pdu++];
		pdu_sess->session_id = pdu_sess_id;
	}

	return num_of_pdu;
}

int pdu_proc_fill_pdu_sess_setup_list(ue_ctx_t *ue_ctx, json_object *js_pdu_sess_list, n3_pdu_sess_t *pdu_sess_list /* x MAX_N3_PDU_NUM */)
{
	int num_of_pdu = 0;

	for (int i = 0; i < json_object_array_length(js_pdu_sess_list) && num_of_pdu < MAX_N3_PDU_NUM; i++) {
		/* get item */
		json_object *js_elem = json_object_array_get_idx(js_pdu_sess_list, i);

		/* check mandatory */
		int pdu_sess_id						= ngap_get_pdu_sess_id(js_elem);
		const char *pdu_nas_pdu 			= ngap_get_pdu_sess_nas_pdu(js_elem);
		int64_t pdu_ambr_dl					= ngap_get_pdu_sess_ambr_dl(js_elem);
		int64_t pdu_ambr_ul					= ngap_get_pdu_sess_ambr_ul(js_elem);
		const char *gtp_te_addr 			= ngap_get_pdu_gtp_te_addr(js_elem);
		const char *gtp_te_id				= ngap_get_pdu_gtp_te_id(js_elem);
		const char *pdu_sess_type			= ngap_get_pdu_sess_type(js_elem);
		json_object *js_pdu_qos_flow_ids	= ngap_get_pdu_qos_flow_ids(js_elem); /* its array */

		if (pdu_sess_id	< 0 || 
				pdu_nas_pdu	== NULL || 
				pdu_ambr_dl	< 0 || 
				pdu_ambr_ul	< 0 || 
				gtp_te_addr	== NULL || 
				gtp_te_id == NULL || 
				pdu_sess_type == NULL || 
				js_pdu_qos_flow_ids == NULL) {
			ERRLOG(LLE, FL, "TODO %s() check mandatory fail!\n", __func__);
			continue;
		}

		n3_pdu_sess_t *pdu_sess = &pdu_sess_list[num_of_pdu++];
		pdu_sess->session_id = pdu_sess_id;
		pdu_sess->pdu_sess_ambr_dl = pdu_ambr_dl;
		pdu_sess->pdu_sess_ambr_ul = pdu_ambr_ul;
		ipaddr_hex_to_inet(gtp_te_addr, pdu_sess->gtp_te_addr);
		sprintf(pdu_sess->gtp_te_id, "%.8s", gtp_te_id);
		pdu_sess->address_family = !strcmp(pdu_sess_type, "ipv6") ? AF_INET6 : AF_INET;

		pdu_sess->qos_flow_id_num = pdu_proc_fill_qos_flow_ids(pdu_sess, js_pdu_qos_flow_ids);

		pdu_ctx_t *pdu_ctx = create_pdu_ctx(pdu_sess_id, PS_PREPARE, pdu_nas_pdu);
		ue_ctx->pdu_ctx_list = g_slist_append(ue_ctx->pdu_ctx_list, pdu_ctx);
	}

	return num_of_pdu;
}

void pdu_proc_sess_establish_accept(pdu_ctx_t *pdu_ctx, void *arg)
{
	ue_ctx_t *ue_ctx = (ue_ctx_t *)arg;

	ERRLOG(LLE, FL, "%s() ue [%s] tcp write pdu (id:%d) accept nas message\n", __func__, UE_TAG(ue_ctx),  pdu_ctx->pdu_sess_id);

	pdu_ctx->pdu_state = PS_CREATED;

	tcp_send_downlink_nas(ue_ctx, pdu_ctx->pdu_nas_pdu);

	free(pdu_ctx->pdu_nas_pdu);
	pdu_ctx->pdu_nas_pdu = NULL;
}

void pdu_proc_json_id_attach(pdu_ctx_t *pdu_ctx, void *arg)
{
	json_object *js_pdu_session_val = (json_object *)arg;

	json_object *js_pdu_id = json_object_new_object();
	json_object_object_add(js_pdu_id, "pDUSessionID", json_object_new_int(pdu_ctx->pdu_sess_id));

	json_object_array_add(js_pdu_session_val, js_pdu_id);
}

gint pdu_proc_find_ctx(gpointer data, gpointer arg)
{
	pdu_ctx_t *pdu_ctx = (pdu_ctx_t *)data;
	uint8_t *find_id = (uint8_t *)arg;

	return pdu_ctx->pdu_sess_id - *find_id;
}

void pdu_proc_remove_ctx(ue_ctx_t *ue_ctx, n3_pdu_info_t *pdu_info)
{
	for (int i = 0; i < pdu_info->pdu_num && i < MAX_N3_PDU_NUM; i++) {
		n3_pdu_sess_t *pdu_sess = &pdu_info->pdu_sessions[i];

		GSList *item = g_slist_find_custom(ue_ctx->pdu_ctx_list, &pdu_sess->session_id, (GCompareFunc)pdu_proc_find_ctx);
		if (item != NULL) {
			pdu_ctx_t *pdu_ctx = (pdu_ctx_t *)item->data;

			if (pdu_ctx->pdu_nas_pdu != NULL) {
				free(pdu_ctx->pdu_nas_pdu);
				pdu_ctx->pdu_nas_pdu = NULL;
			}
			free(item->data);
			ue_ctx->pdu_ctx_list = g_slist_remove(ue_ctx->pdu_ctx_list, item->data);
		}
	}
}

void pdu_proc_flush_ctx(ue_ctx_t *ue_ctx)
{
	while (ue_ctx->pdu_ctx_list && ue_ctx->pdu_ctx_list->data) {

		pdu_ctx_t *pdu_ctx = (pdu_ctx_t *)ue_ctx->pdu_ctx_list->data;
		if (pdu_ctx->pdu_nas_pdu != NULL) {
			free(pdu_ctx->pdu_nas_pdu);
			pdu_ctx->pdu_nas_pdu = NULL;
		}

		free(ue_ctx->pdu_ctx_list->data);
		ue_ctx->pdu_ctx_list = g_slist_remove(ue_ctx->pdu_ctx_list, ue_ctx->pdu_ctx_list->data);
	}
}
