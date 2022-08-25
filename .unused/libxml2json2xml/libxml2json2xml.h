#include <stdio.h>
#include <string.h>
#include <libxml/tree.h>
#include <json-c/json.h>

#include <asn_internal.h>
#include <ENUMERATED.h>
#include <NativeInteger.h>

#define JSON_C_PRETTY_NOSLASH   (JSON_C_TO_STRING_PRETTY|JSON_C_TO_STRING_NOSLASHESCAPE)
#define JSON_PRINT(jobj)        json_object_to_json_string_ext(jobj, JSON_C_PRETTY_NOSLASH)

/* ------------------------- json2xml.c --------------------------- */
void    attach_xml_obj(json_object *jobj, xmlNode *cur, int parent_type, const char *parent_name);
void    json2xml_convert_object(json_object *jobj, xmlNode *cur, int parent_type, const char *parent_name);
int     json2xml_sample(int argc, char **argv);

/* ------------------------- xml2json.c --------------------------- */
json_object     *attach_json_obj(xmlNode *cur_node, const char *cur_node_name, json_object *jobj, int is_num, int is_leaf);
void    xml2json_convert_elements(xmlNode *anode, json_object *jobj, asn_TYPE_descriptor_t *asn_td);
int     xml2json_sample(int argc, char **argv);
