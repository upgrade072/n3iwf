#include <nwucp.h>

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
