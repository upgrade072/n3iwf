#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libconfig.h>

#include <json-c/json.h>
#include <json-c/json_object.h>

#include <libutil.h>
#include <json_macro.h>

#define JS_TEST "{\"successfulOutcome\":{\"procedureCode\":41,\"criticality\":\"reject\",\"value\":{\"protocolIEs\":[{\"id\":10,\"criticality\":\"ignore\",\"value\":17179977417},{\"id\":85,\"criticality\":\"ignore\",\"value\":10021},{\"id\":121,\"criticality\":\"reject\",\"value\":{\"userLocationInformationN3IWF\":{\"iPAddress\":{\"value\":\"C0A87F02\",\"length\":32},\"portNumber\":\"01F4\"}}},{\"id\":60,\"criticality\":\"reject\",\"value\":[]}]}}}"
int main()
{
	json_object *js_test = json_tokener_parse(JS_TEST);

	fprintf(stderr, "%s\n", JS_PRINT_PRETTY(js_test));

	json_object_put(js_test);
}
