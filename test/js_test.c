#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libconfig.h>

#include <json-c/json.h>
#include <json-c/json_object.h>

#include <libutil.h>
#include <libdata.h>
#include <json_macro.h>

int main()
{
	size_t read_size = 0;
	char *js_text = file_to_buffer("./test.json", "r", &read_size);
	json_object *js_tok = json_tokener_parse(js_text);

	key_list_t key_list = {0,};
	json_object *js_value = search_json_object_ex(js_tok, "/NGAP-PDU/*/value/*/protocolIEs/*/value/RAN-UE-NGAP-ID", &key_list);

	int value = json_object_get_int(js_value);
	fprintf(stderr, "{dbg} [%s] %d depth=(%d)\n", js_value == NULL ? "nil" : "exist", value, key_list.depth);

	for (int i = 0; i < key_list.key_num; i++) {
		fprintf(stderr, "{dbg} %d %s\n", i, key_list.key_val[i]);
	}

	json_object_put(js_tok);
	free(js_text);
}
