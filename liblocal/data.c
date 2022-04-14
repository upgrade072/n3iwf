#include "libdata.h"

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
