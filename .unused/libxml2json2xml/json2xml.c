#include "libxml2json2xml.h"

void attach_xml_obj(json_object *jobj, xmlNode *cur, int parent_type, const char *parent_name)
{
	char cnvt_buf[1024] = {0,};
	const char *str = NULL;

	int json_type = json_object_get_type(jobj);

	if (json_type == json_type_boolean) {
		sprintf(cnvt_buf, "%d", json_object_get_boolean(jobj));
	} else if (json_type == json_type_double) {
		sprintf(cnvt_buf, "%lf", json_object_get_double(jobj));
	} else if (json_type == json_type_int) {
		sprintf(cnvt_buf, "%ld", json_object_get_int64(jobj));
	} else if (json_type ==  json_type_string) {
		str = json_object_get_string(jobj);
	}

	if (json_type != json_type_string) {
		str = cnvt_buf;
	}

	xmlNodeAddContent(cur, (xmlChar *)str);
}

void json2xml_convert_object(json_object *jobj, xmlNode *cur, int parent_type, const char *parent_name)
{
	int json_type = json_object_get_type(jobj);

	if (json_type == json_type_object) {
		json_object_object_foreach(jobj, key, val) {
			xmlNode *new = xmlNewChild(cur, NULL, (xmlChar *)key, (xmlChar *)NULL);
			json2xml_convert_object(val, new, json_type, key);
		}
	} else if (json_type == json_type_array) {
		xmlNode *new_current = cur;
		if (cur->parent != NULL) {
			new_current = cur->parent;
			xmlUnlinkNode(cur);
			xmlFreeNode(cur);
		}
		for (int i = 0; i < json_object_array_length(jobj); i++) {
			json_object *jelem = json_object_array_get_idx(jobj, i);
			json_type = json_object_get_type(jelem);
			xmlNode *new_node = xmlNewChild(new_current, NULL, parent_name != NULL ? (xmlChar *)parent_name : (xmlChar *)"object", (xmlChar *)NULL);
			json2xml_convert_object(jelem, new_node, json_type, parent_name);
		}
	} else {
		attach_xml_obj(jobj, cur, parent_type, parent_name);
	}
}

int json2xml_sample(int argc, char **argv)
{
	xmlKeepBlanksDefault(0);
	xmlIndentTreeOutput = 2;

	xmlDoc *doc = xmlNewDoc(NULL);
	xmlNode *xml = xmlNewNode(NULL, (xmlChar *)"packet");

	json_object *jobj = json_object_from_file(argv[1]);
	json2xml_convert_object(jobj, xml, json_type_object, NULL);
	json_object_put(jobj);

	xmlDocSetRootElement(doc, xml);
	xmlSaveFormatFile("./result.xml", doc, 2);
	xmlFreeDoc(doc);
	xmlCleanupParser();

	return 0;
}
