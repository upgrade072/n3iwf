#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libconfig.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <libxml2json2xml.h>

#include <libutil.h>
#include <libdata.h>

/* for ASN.1 NGAP PDU */
#include <NGAP-PDU.h>
#include <ProcedureCode.h>
#include <ProtocolIE-ID.h>

#define DEBUG_ASN   1

#define NGSetupRequest_JSON  "{\"NGAP-PDU\":{\"initiatingMessage\":{\"procedureCode\":%ld,\"criticality\":{\"reject\":\"\"},\"value\":{\"NGSetupRequest\":{\"protocolIEs\":{\"NGSetupRequestIEs\":[{\"id\":%ld,\"criticality\":{\"reject\":\"\"},\"value\":{\"GlobalRANNodeID\":{\"globalN3IWF-ID\":{\"pLMNIdentity\":\"%s\",\"n3IWF-ID\":{\"n3IWF-ID\":\"%s\"}}}}},{\"id\":%ld,\"criticality\":{\"ignore\":\"\"},\"value\":{\"RANNodeName\":\"%s\"}},{\"id\":%ld,\"criticality\":{\"reject\":\"\"},\"value\":{\"SupportedTAList\":{ }}}]}}}}}}"

config_t MAIN_CFG;

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

int main() {
    config_init (&MAIN_CFG);
    if (!config_read_file(&MAIN_CFG, "./test.cfg")) {
        fprintf(stderr, "cfg err!\n");
        fprintf(stderr, "%s() fatal! fail to load cfg file=(%s) line:text(%d/%s)!\n",
                __func__,
                config_error_file(&MAIN_CFG),
                config_error_line(&MAIN_CFG),
                config_error_text(&MAIN_CFG));
    } else {
        config_set_options(&MAIN_CFG, 0);
        config_write(&MAIN_CFG, stderr);
    }           
            
    /* cnvt whole profile to JSON format */ 
    json_object *json_cfg = json_object_new_object();
    config_setting_t *cfg_n3iwf_profile = config_lookup(&MAIN_CFG, "n3iwf_profile");
    cnvt_cfg_to_json(json_cfg, cfg_n3iwf_profile, CONFIG_TYPE_GROUP);
        
    /* load some info from CFG */
    const char *mcc_mnc = JS_SEARCH_STR(json_cfg, "/n3iwf_profile/global_info/mcc_mnc");
    int n3iwf_id = JS_SEARCH_INT(json_cfg, "/n3iwf_profile/global_info/n3iwf_id");
    const char *node_name = JS_SEARCH_STR(json_cfg, "/n3iwf_profile/global_info/node_name");
    json_object *js_support_ta_item = JS_SEARCH_OBJ(json_cfg, "/n3iwf_profile/SupportedTAItem");
                
    /* OK we get ngap_setup_request JSON PDU */
    json_object *js_ng_setup_request = create_ng_setup_request_json(mcc_mnc, n3iwf_id, node_name, js_support_ta_item);
            
    fprintf(stderr, "%s\n", JS_PRINT_PRETTY(js_ng_setup_request));
            
    /* convert it to XML */
    size_t xmlb_size = 0;
    xmlBuffer *xmlb = convert_json_to_xml(js_ng_setup_request, "NGAP-PDU", &xmlb_size, 1);
    
    /* create asn_ctx from XML(XER) */
    NGAP_PDU_t *ngap_ng_setup_request = decode_pdu_to_ngap_asn(ATS_CANONICAL_XER, (char *)xmlb->content, xmlb_size, 0);
    xmlBufferFree(xmlb); // free xml manually
    check_asn_constraints(asn_DEF_NGAP_PDU, ngap_ng_setup_request, DEBUG_ASN);
    
    /*  encode APER from asn_ctx */
    size_t aper_size = 0;
    char *aper_string = encode_asn_to_pdu_buffer(ATS_ALIGNED_BASIC_PER, asn_DEF_NGAP_PDU, ngap_ng_setup_request, &aper_size, 1);
    
    /* save pdu to file */
    buffer_to_file("./result.bin", "wb", aper_string, aper_size, 1);

    json_object_put(json_cfg);
    config_destroy(&MAIN_CFG);
    xmlCleanupParser();
}   
