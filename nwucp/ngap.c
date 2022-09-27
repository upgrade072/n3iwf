#include <nwucp.h>
extern main_ctx_t *MAIN_CTX;

void ngap_send_json(char *hostname, json_object *js_ngap)
{
	ngap_msg_t msg = { .mtype = 1, }, *ngap_msg = &msg;
	sprintf(ngap_msg->sctp_tag.hostname, "%s", hostname);
	ngap_msg->sctp_tag.stream_id = 0;
	ngap_msg->sctp_tag.ppid = SCTP_PPID_NGAP;
	ngap_msg->msg_size = sprintf(ngap_msg->msg_body, "%s", JS_PRINT_COMPACT(js_ngap));

	fprintf(stderr, "%s() send to amf=[%s]\n%s\n", __func__, hostname, JS_PRINT_PRETTY(js_ngap));

	int res = msgsnd(MAIN_CTX->QID_INFO.nwucp_ngapp_qid, ngap_msg, NGAP_MSG_SIZE(ngap_msg), IPC_NOWAIT);
	fprintf(stderr, "{dbg} %s msgsnd res=(%d) size=(%ld) ",  __func__, res, ngap_msg->msg_size);
	if (res < 0) {
		fprintf(stderr, "(%d:%s)\n", errno, strerror(errno));
	} else {
		fprintf(stderr, "ngap_size=(%ld)\n", NGAP_MSG_SIZE(ngap_msg));
	}
}

#define NGSetupRequest_JSON "{\"initiatingMessage\":{\"procedureCode\":21,\"criticality\":\"reject\",\"value\":{\"protocolIEs\":[{\"id\":27,\"criticality\":\"reject\",\"value\":{\"globalN3IWF-ID\":{\"pLMNIdentity\":\"%s\",\"n3IWF-ID\":{\"n3IWF-ID\":\"%s\"}}}},{\"id\":82,\"criticality\":\"ignore\",\"value\":\"%s\"},{\"id\":102,\"criticality\":\"reject\"}]}}}"
json_object *create_ng_setup_request_json(const char *mnc_mcc, int n3iwf_id, const char *ran_node_name, json_object *js_support_ta_item)
{
	/* make VAR */
    char plmn_id[6 + 1] = {0,};
    print_bcd_str(mnc_mcc, plmn_id, sizeof(plmn_id));
    char n3iwf_id_str[16 + 1] = {0,};
	sprintf(n3iwf_id_str, "%.4x", n3iwf_id);

	/* make NGSetupRequest */
	char *ptr = NULL;
	asprintf(&ptr, NGSetupRequest_JSON, plmn_id, n3iwf_id_str, ran_node_name);
    json_object *js_ng_setup_request = json_tokener_parse(ptr);
    free(ptr);

	/* get <id:102 obj> SupportedTAList */
    json_object *js_supported_ta_list = JS_SEARCH_OBJ(js_ng_setup_request, "/initiatingMessage/value/protocolIEs/2");

    /* value from CFG, so we copy deep */
    json_object *js_deep_supported_ta_list = NULL;
    json_object_deep_copy(js_support_ta_item, &js_deep_supported_ta_list, NULL);
    json_object_object_add(js_supported_ta_list, "value", js_deep_supported_ta_list);

	return js_ng_setup_request;
}

#define InitialUEMessage_JSON "{\"initiatingMessage\":{\"procedureCode\":15,\"criticality\":\"ignore\",\"value\":{\"protocolIEs\":[{\"id\":85,\"criticality\":\"reject\",\"value\":%d},{\"id\":38,\"criticality\":\"reject\",\"value\":\"%s\"},{\"id\":121,\"criticality\":\"reject\",\"value\":{\"userLocationInformationN3IWF\":{\"iPAddress\":{\"value\":\"%s\",\"length\":32},\"portNumber\":\"%04X\"}}},{\"id\":90,\"criticality\":\"ignore\",\"value\":\"%s\"},{\"id\":112,\"criticality\":\"ignore\",\"value\":\"requested\"}]}}}"
json_object *create_initial_ue_message_json(int ue_id, char *nas_pdu, char *ip_str, int port, char *cause)
{
	/* make VAR */
	char ip_hex[128] = {0,};
	ipaddr_to_hex(ip_str, ip_hex);

	/* make InitialUEMessage */
	char *ptr = NULL;
	asprintf(&ptr, InitialUEMessage_JSON, ue_id, nas_pdu, ip_hex, port, cause);
    json_object *js_initial_ue_message = json_tokener_parse(ptr);
    free(ptr);

	return js_initial_ue_message;
}

#define UplinkNASTransport_JSON "{\"initiatingMessage\":{\"procedureCode\":46,\"criticality\":\"ignore\",\"value\":{\"protocolIEs\":[{\"id\":10,\"criticality\":\"reject\",\"value\":%d},{\"id\":85,\"criticality\":\"reject\",\"value\":%d},{\"id\":38,\"criticality\":\"reject\",\"value\":\"%s\"},{\"id\":121,\"criticality\":\"ignore\",\"value\":{\"userLocationInformationN3IWF\":{\"iPAddress\":{\"value\":\"%s\",\"length\":32},\"portNumber\":\"%04X\"}}}]}}}"
json_object *create_uplink_nas_transport_json(int amf_ue_id, int ran_ue_id, const char *nas_pdu, char *ip_str, int port)
{
	/* make VAR */
	char ip_hex[128] = {0,};
	ipaddr_to_hex(ip_str, ip_hex);

	/* make UplinkNASTransport */
	char *ptr = NULL;
	asprintf(&ptr, UplinkNASTransport_JSON, amf_ue_id, ran_ue_id, nas_pdu, ip_hex, port);
    json_object *js_uplink_nas_transport_message = json_tokener_parse(ptr);
    free(ptr);

	return js_uplink_nas_transport_message;
}

#define InitialContextSetupResponse_JSON "{\"%s\":{\"procedureCode\":14,\"criticality\":\"reject\",\"value\":{\"protocolIEs\":[{\"id\":10,\"criticality\":\"ignore\",\"value\":%d},{\"id\":85,\"criticality\":\"ignore\",\"value\":%d}]}}}"
json_object *create_initial_context_setup_response_json(const char *result, int amf_ue_id, int ran_ue_id)
{
	/* make InitialContextSetupResponse */
	char *ptr = NULL;
	asprintf(&ptr, InitialContextSetupResponse_JSON, result, amf_ue_id, ran_ue_id);
	json_object *js_initial_context_setup_response_message = json_tokener_parse(ptr);
	free(ptr);

	return js_initial_context_setup_response_message;
}

#define PDUSessionResourceSetupResponse_JSON "{\"%s\":{\"procedureCode\":29,\"criticality\":\"reject\",\"value\":{\"protocolIEs\":[{\"id\":10,\"criticality\":\"ignore\",\"value\":%d},{\"id\":85,\"criticality\":\"ignore\",\"value\":%d},{\"id\":75,\"criticality\":\"ignore\",\"value\":[]}]}}}"
#define PDUSessionItem_JSON "{\"pDUSessionID\":%d\"pDUSessionResourceSetupResponseTransfer\":{\"containing\":{\"dLQosFlowPerTNLInformation\":{\"uPTransportLayerInformation\":{\"gTPTunnel\":{\"transportLayerAddress\":{\"value\":\"%s\",\"length\":32},\"gTP-TEID\":\"%.8s\"}},\"associatedQosFlowList\":[]}}}}"
json_object *create_pdu_session_resource_setup_response_json(const char *result, int amf_ue_id, int ran_ue_id, n3_pdu_info_t *pdu_info)
{
	/* make PDUSessionResourceSetupResponse */
	char *msg_ptr = NULL;
	asprintf(&msg_ptr, PDUSessionResourceSetupResponse_JSON, result, amf_ue_id, ran_ue_id);
	json_object *js_pdu_session_resource_setup_response_message = json_tokener_parse(msg_ptr);
	free(msg_ptr);

	/* if fail result, no more body need */
	if (!strcmp(result, "UnSuccessfulOutcome")) {
		return js_pdu_session_resource_setup_response_message;
	}

	key_list_t key_pdu_session_val = {0,};
	json_object *js_pdu_session_val = 
		search_json_object_ex(js_pdu_session_resource_setup_response_message, 
			"/*/value/protocolIEs/{id:75, value}", &key_pdu_session_val);

	for (int i = 0; i < pdu_info->pdu_num && i < MAX_N3_PDU_NUM; i++) {
		n3_pdu_sess_t *pdu_sess = &pdu_info->pdu_sessions[i];

		char ip_addr[INET_ADDRSTRLEN] = {0,};
		ipaddr_to_hex(pdu_sess->gtp_te_addr, ip_addr);

		/* make pdu session item */
		char *item_ptr = NULL;
		asprintf(&item_ptr, PDUSessionItem_JSON, pdu_sess->session_id, ip_addr, pdu_sess->gtp_te_id);
		json_object *js_pdu_item = json_tokener_parse(item_ptr);
		free(item_ptr);

		json_object *js_qos_flow_ids = search_json_object(js_pdu_item, 
				"/pDUSessionResourceSetupResponseTransfer/containing/dLQosFlowPerTNLInformation/associatedQosFlowList");

		/* make qos flow ids */
		for (int k = 0; k < pdu_sess->qos_flow_id_num && k < MAX_N3_QOS_FLOW_IDS; k++) {
			json_object *js_flow_id = json_object_new_object();
			json_object_object_add(js_flow_id, "qosFlowIdentifier", json_object_new_int(pdu_sess->qos_flow_id[k]));
			json_object_array_add(js_qos_flow_ids, js_flow_id);
		}

		json_object_array_add(js_pdu_session_val, js_pdu_item);
	}

	fprintf(stderr, "{DBG} %s()\n%s\n", __func__, JS_PRINT_PRETTY(js_pdu_session_resource_setup_response_message));

	return js_pdu_session_resource_setup_response_message;
}
