#ifndef __LIBDATA_H__
#define __LIBDATA_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <libconfig.h>
#include <json-c/json.h>
#include <json-c/json_object.h>

#include <libxml2json2xml.h>
#include <NGAP-PDU.h>

#define JS_SEARCH_OBJ(json,key) search_json_object(json,key)
#define JS_SEARCH_STR(json,key) json_object_get_string(search_json_object(json,key))
#define JS_SEARCH_INT(json,key) json_object_get_int(search_json_object(json,key))
#define JS_PRINT_PRETTY(json) json_object_to_json_string_ext(json, JSON_C_TO_STRING_PRETTY|JSON_C_TO_STRING_NOSLASHESCAPE)

/* ------------------------- data.c --------------------------- */
int     cnvt_cfg_to_json(json_object *obj, config_setting_t *setting, int callerType);
int     check_number(char *ptr);
json_object     *search_json_object(json_object *obj, char *key_string);
int     check_asn_constraints(asn_TYPE_descriptor_t type_desc, const void *sptr, int debug);
NGAP_PDU_t      *decode_pdu_to_ngap_asn(enum asn_transfer_syntax syntax, char *pdu, size_t pdu_size, int free_pdu);
char    *encode_asn_to_pdu_buffer(enum asn_transfer_syntax syntax, asn_TYPE_descriptor_t type_desc, void *sptr, size_t *encode_size, int free_asn);
json_object     *convert_xml_to_jobj(char *xml_string, size_t xml_size, asn_TYPE_descriptor_t asn_td, int free_xml);
xmlBuffer       *convert_json_to_xml(json_object *jobj, const char *pdu_name, size_t *encoded_size, int free_jobj);

#endif /* __LIBDATA_H__ */
