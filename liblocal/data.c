#include "libdata.h"

int cnvt_cfg_to_json(json_object *obj, config_setting_t *setting, int callerType)
{   
    json_object *obj_group = NULL;
    json_object *obj_array = NULL;
    int count = 0;
    
    switch (setting->type) {
        // SIMPLE CASE
        case CONFIG_TYPE_INT:
            json_object_object_add(obj, setting->name, json_object_new_int(setting->value.ival));                                                                                  
            break;
        case CONFIG_TYPE_STRING:
            if (callerType == CONFIG_TYPE_ARRAY || callerType == CONFIG_TYPE_LIST) {
                json_object_array_add(obj, json_object_new_string(setting->value.sval));                                                                                           
            } else {
                json_object_object_add(obj, setting->name, json_object_new_string(setting->value.sval));                                                                           
            }
            break;
        case CONFIG_TYPE_BOOL:
            json_object_object_add(obj, setting->name, json_object_new_boolean(setting->value.ival));                                                                              
            break;
        // COMPLEX CASE
        case CONFIG_TYPE_GROUP:
            obj_group = json_object_new_object();    
            count = config_setting_length(setting);  
            for (int i = 0; i < count; i++) {            
                config_setting_t *elem = config_setting_get_elem(setting, i);
                cnvt_cfg_to_json(obj_group, elem, setting->type);                                                                                                                  
            }
            if (callerType == CONFIG_TYPE_ARRAY || callerType == CONFIG_TYPE_LIST) {
                json_object_array_add(obj, obj_group); 
            } else {
                json_object_object_add(obj, setting->name, obj_group);                                                                                                             
            }
            break;
        case CONFIG_TYPE_ARRAY:
        case CONFIG_TYPE_LIST:
            obj_array = json_object_new_array();
            json_object_object_add(obj, setting->name, obj_array);

            count = config_setting_length(setting);
            for (int i = 0; i < count; i++) {
                config_setting_t *elem = config_setting_get_elem(setting, i);
                cnvt_cfg_to_json(obj_array, elem, setting->type);
            }
            break;
        default:
            fprintf(stderr, "TODO| do something!\n");
            break;
    }

    return 0;
}

int check_number(char *ptr)
{
    for (int i = 0; i < strlen(ptr); i++) {
        if (isdigit(ptr[i]) == 0)
            return -1;
    }
    return atoi(ptr);
}

json_object *search_json_object(json_object *obj, char *key_string)
{
    char *key_parse = strdup(key_string);

    char *ptr = strtok(key_parse, "/");
    json_object *input = obj;
    json_object *output = NULL;

    while (ptr != NULL) {
        int cnvt_num = check_number(ptr);

        if (cnvt_num >= 0) {
            if (json_object_get_type(input) != json_type_array)
                goto SJO_ERR;
            if ((output = json_object_array_get_idx(input, cnvt_num)) == NULL)
                goto SJO_ERR;
        } else {
            if (json_object_object_get_ex(input, ptr, &output) == 0)
                goto SJO_ERR;
        }
        input = output;
        ptr = strtok(NULL, "/");
    }
    free(key_parse);
    return output;

SJO_ERR:
    free(key_parse);
    return NULL;
}

int check_asn_constraints(asn_TYPE_descriptor_t type_desc, const void *sptr, int debug)
{   
    char errbuf[128] = {0,}; 
    size_t errlen = sizeof(errbuf);
    
    int ret = asn_check_constraints(&type_desc, sptr, errbuf, &errlen);
    //fprintf(stderr, "[%s] Check Constraint for %s, Result: %s Err=(%s)\n", __func__, type_desc.name, ret ? "NOK" : "OK", errbuf);
    
    if (debug) {
        asn_fprint(stdout, &type_desc, sptr);
        xer_fprint(stdout, &type_desc, sptr);
    }
    
    return ret;
}

NGAP_PDU_t *decode_pdu_to_ngap_asn(enum asn_transfer_syntax syntax, char *pdu, size_t pdu_size, int free_pdu)
{
    NGAP_PDU_t *pdu_payload_asn = NULL;

    asn_dec_rval_t dc_res = asn_decode(0, syntax, &asn_DEF_NGAP_PDU, (void **)&pdu_payload_asn, pdu, pdu_size);
    //fprintf(stderr, "[%s] Decode %s, bytes=(%ld) rcode=(%d)\n", __func__, dc_res.code == RC_OK ? "OK" : "NOK", dc_res.consumed, dc_res.code);

    if (dc_res.code != RC_OK)
        ASN_STRUCT_FREE(asn_DEF_NGAP_PDU, pdu_payload_asn);

    if (free_pdu)
        free(pdu);

    return dc_res.code != RC_OK ? NULL : pdu_payload_asn;
}

char *encode_asn_to_pdu_buffer(enum asn_transfer_syntax syntax, asn_TYPE_descriptor_t type_desc, void *sptr, size_t *encode_size, int free_asn)
{
    asn_encode_to_new_buffer_result_t encode_res = asn_encode_to_new_buffer(0, syntax, &type_desc, sptr);
    //fprintf(stderr, "[%s] Encode %s, bytes=(%ld)\n", __func__, encode_res.buffer == NULL ? "NOK" : "OK", encode_res.result.encoded);

    if (free_asn)
        ASN_STRUCT_FREE(type_desc, sptr);

    *encode_size = encode_res.result.encoded;

    return encode_res.buffer == NULL ? NULL : encode_res.buffer;
}

json_object *convert_xml_to_jobj(char *xml_string, size_t xml_size, asn_TYPE_descriptor_t asn_td, int free_xml)
{
    json_object *jobj = json_object_new_object();

    xmlDoc *xml_doc = xmlReadMemory(xml_string, xml_size, NULL, NULL, 0);
    xmlNode *xml_node = xmlDocGetRootElement(xml_doc);
    xml2json_convert_elements(xml_node, jobj, &asn_td);

    xmlFreeDoc(xml_doc); // free include xmlNode

    fprintf(stderr, "%s\n", JSON_PRINT(jobj));

    if (free_xml)
        free(xml_string);

    return jobj;
}

xmlBuffer *convert_json_to_xml(json_object *jobj, const char *pdu_name, size_t *encoded_size, int free_jobj)
{
    /* find NGAP-PDU & create to XML */
    json_object *jobj_ngap_pdu = json_object_object_get(jobj, pdu_name);
    xmlNode *xml_node = xmlNewNode(NULL, (xmlChar *)pdu_name);
    json2xml_convert_object(jobj_ngap_pdu, xml_node, json_type_object, NULL);
    xmlBuffer *xml_buffer = xmlBufferCreate();
    int xml_size = xmlNodeDump(xml_buffer, NULL, xml_node, 0, 0);

    xmlFreeNode(xml_node);

    if (free_jobj)
        json_object_put(jobj);

    *encoded_size = xml_size;

    return xml_buffer;
}
