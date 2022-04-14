#include <nwucp.h>

#define NGSetupRequest_JSON  "{\"NGAP-PDU\":{\"initiatingMessage\":{\"procedureCode\":%ld,\"criticality\":{\"reject\":\"\"},\"value\":{\"NGSetupRequest\":{\"protocolIEs\":{\"NGSetupRequestIEs\":[{\"id\":%ld,\"criticality\":{\"reject\":\"\"},\"value\":{\"GlobalRANNodeID\":{\"globalN3IWF-ID\":{\"pLMNIdentity\":\"%s\",\"n3IWF-ID\":{\"n3IWF-ID\":\"%s\"}}}}},{\"id\":%ld,\"criticality\":{\"ignore\":\"\"},\"value\":{\"RANNodeName\":\"%s\"}},{\"id\":%ld,\"criticality\":{\"reject\":\"\"},\"value\":{\"SupportedTAList\":{ }}}]}}}}}}"
json_object *create_ng_setup_request_json(const char *mnc_mcc, int n3iwf_id, const char *ran_node_name, json_object *js_support_ta_item)
{
    char plmn_id[6 + 1] = {0,};
    print_bcd_str(mnc_mcc, plmn_id, sizeof(plmn_id));
    char n3iwf_id_str[16 + 1] = {0,};
    print_byte_bin(n3iwf_id, n3iwf_id_str, sizeof(n3iwf_id_str));

    char *ptr = NULL; size_t ptr_size = 0;
    FILE *fp = open_memstream(&ptr, &ptr_size);
    fprintf(fp, NGSetupRequest_JSON,
                ProcedureCode_id_NGSetup,
                    ProtocolIE_ID_id_GlobalRANNodeID,
                        plmn_id,
                        n3iwf_id_str,
                    ProtocolIE_ID_id_RANNodeName,
                        ran_node_name,
                    ProtocolIE_ID_id_SupportedTAList);
    fclose(fp);
    json_object *js_ng_setup_request_json = json_tokener_parse(ptr);
    free(ptr);

    json_object *js_supported_ta_list = JS_SEARCH_OBJ(js_ng_setup_request_json,
            "/NGAP-PDU/initiatingMessage/value/NGSetupRequest/protocolIEs/NGSetupRequestIEs/2/value/SupportedTAList");

    /* value from CFG, so we copy deep */
    json_object *js_deep_supported_ta_list = NULL;
    json_object_deep_copy(js_support_ta_item, &js_deep_supported_ta_list, NULL);
    json_object_object_add(js_supported_ta_list, "SupportedTAItem", js_deep_supported_ta_list);

    return js_ng_setup_request_json;
}
