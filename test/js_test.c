#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libconfig.h>

#include <json-c/json.h>
#include <json-c/json_object.h>

#include <libutil.h>
#include <json_macro.h>

#define PDUSessionResourceSetupResponse_JSON "{\"successfulOutcome\":{\"procedureCode\":29,\"criticality\":\"reject\",\"value\":{\"protocolIEs\":[{\"id\":10,\"criticality\":\"ignore\",\"value\":1},{\"id\":85,\"criticality\":\"ignore\",\"value\":0},{\"id\":75,\"criticality\":\"ignore\",\"value\":[]}]}}}"

#define PDUSessionItem_JSON "{\"pDUSessionID\":1,\"pDUSessionResourceSetupResponseTransfer\":{\"containing\":{\"dLQosFlowPerTNLInformation\":{\"uPTransportLayerInformation\":{\"gTPTunnel\":{\"transportLayerAddress\":{\"value\":\"0AC8C802\",\"length\":32},\"gTP-TEID\":\"00000001\"}},\"associatedQosFlowList\":[]}}}}"

int main()
{
	json_object *js_pdu_session_resource_setup_response = json_tokener_parse(PDUSessionResourceSetupResponse_JSON);

	json_object *js_pdu_session_item = json_tokener_parse(PDUSessionItem_JSON);

	key_list_t key_pdu_session_val = {0,};
	json_object *js_pdu_session_val = search_json_object_ex(js_pdu_session_resource_setup_response, "/*/value/protocolIEs/{id:75, value}", &key_pdu_session_val);

	json_object *js_qos_flow_ids = search_json_object(js_pdu_session_item, "/pDUSessionResourceSetupResponseTransfer/containing/dLQosFlowPerTNLInformation/associatedQosFlowList");
	for (int i = 0; i < 3; i++) {
		json_object *js_new = json_object_new_object();
		json_object_object_add(js_new, "qosFlowIdentifier", json_object_new_int(9));
		fprintf(stderr, "%s\n", JS_PRINT_PRETTY(js_new));
		json_object_array_add(js_qos_flow_ids, js_new);
	}

	json_object_array_add(js_pdu_session_val, js_pdu_session_item);

	fprintf(stderr, "%s\n", JS_PRINT_PRETTY(js_pdu_session_resource_setup_response));

	json_object_put(js_pdu_session_resource_setup_response);
}
