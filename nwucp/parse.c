#include <nwucp.h>
extern main_ctx_t *MAIN_CTX;

int ngap_get_amf_ue_ngap_id(json_object *js_ngap_pdu)
{
	key_list_t key_amf_ue_ngap_id = {0,};
	json_object *js_amf_ue_ngap_id = 
		search_json_object_ex(js_ngap_pdu, "/initiatingMessage/value/protocolIEs/{id:10, value}", &key_amf_ue_ngap_id);

	return js_amf_ue_ngap_id == NULL ? -1 : json_object_get_int(js_amf_ue_ngap_id);
}

int ngap_get_ran_ue_ngap_id(json_object *js_ngap_pdu)
{
	key_list_t key_ran_ue_ngap_id = {0,};
	json_object *js_ran_ue_ngap_id = 
		search_json_object_ex(js_ngap_pdu, "/initiatingMessage/value/protocolIEs/{id:85, value}", &key_ran_ue_ngap_id);

	return js_ran_ue_ngap_id == NULL ? -1 : json_object_get_int(js_ran_ue_ngap_id);
}

const char *ngap_get_nas_pdu(json_object *js_ngap_pdu)
{
	key_list_t key_nas_pdu = {0,};
	json_object *js_nas_pdu = 
		search_json_object_ex(js_ngap_pdu, "/initiatingMessage/value/protocolIEs/{id:38, value}/", &key_nas_pdu);

	return js_nas_pdu == NULL ? NULL : json_object_get_string(js_nas_pdu);
}

const char *ngap_get_security_key(json_object *js_ngap_pdu)
{
	key_list_t key_security_key = {0,};
	json_object *js_security_key = 
		search_json_object_ex(js_ngap_pdu, "/initiatingMessage/value/protocolIEs/{id:94, value}/", &key_security_key);

	return js_security_key == NULL ? NULL : json_object_get_string(js_security_key);
}

int64_t ngap_get_ue_ambr_dl(json_object *js_ngap_pdu)
{
	key_list_t key_ue_ambr_dl = {0,};
	json_object *js_ue_ambr_dl = search_json_object_ex(js_ngap_pdu, "/initiatingMessage/value/protocolIEs/{id:110, value}/uEAggregateMaximumBitRateDL", &key_ue_ambr_dl);

	return js_ue_ambr_dl == NULL ? -1 : json_object_get_int64(js_ue_ambr_dl);
}

int64_t ngap_get_ue_ambr_ul(json_object *js_ngap_pdu)
{
	key_list_t key_ue_ambr_ul = {0,};
	json_object *js_ue_ambr_ul = search_json_object_ex(js_ngap_pdu, "/initiatingMessage/value/protocolIEs/{id:110, value}/uEAggregateMaximumBitRateUL", &key_ue_ambr_ul);

	return js_ue_ambr_ul == NULL ? -1 : json_object_get_int64(js_ue_ambr_ul);
}

int ngap_get_pdu_sess_id(json_object *js_pdu_sess_elem)
{
	json_object *js_pdu_sess_id = search_json_object(js_pdu_sess_elem, "/pDUSessionID");

	return js_pdu_sess_id == NULL ? -1 : json_object_get_int(js_pdu_sess_id);
}

const char *ngap_get_pdu_sess_nas_pdu(json_object *js_pdu_sess_elem)
{
	json_object *js_pdu_nas_pdu =  search_json_object(js_pdu_sess_elem, "/pDUSessionNAS-PDU");

	return js_pdu_nas_pdu == NULL ? NULL : json_object_get_string(js_pdu_nas_pdu);
}

int64_t ngap_get_pdu_sess_ambr_dl(json_object *js_pdu_sess_elem)
{
	key_list_t key_pdu_ambr_dl = {0,};
	json_object *js_pdu_ambr_dl = search_json_object_ex(js_pdu_sess_elem, 
			"/pDUSessionResourceSetupRequestTransfer/containing/protocolIEs/{id:130, value}/pDUSessionAggregateMaximumBitRateDL", 
			&key_pdu_ambr_dl);

	return js_pdu_ambr_dl == NULL ? -1 : json_object_get_int64(js_pdu_ambr_dl);
}

int64_t ngap_get_pdu_sess_ambr_ul(json_object *js_pdu_sess_elem)
{
	key_list_t key_pdu_ambr_ul = {0,};
	json_object *js_pdu_ambr_ul = search_json_object_ex(js_pdu_sess_elem, 
			"/pDUSessionResourceSetupRequestTransfer/containing/protocolIEs/{id:130, value}/pDUSessionAggregateMaximumBitRateUL", 
			&key_pdu_ambr_ul);

	return js_pdu_ambr_ul == NULL ? -1 : json_object_get_int64(js_pdu_ambr_ul);
}

const char *ngap_get_pdu_gtp_te_addr(json_object *js_pdu_sess_elem)
{
	key_list_t key_gtp_te_addr = {0,};
	json_object *js_gtp_te_addr = search_json_object_ex(js_pdu_sess_elem,
			"/pDUSessionResourceSetupRequestTransfer/containing/protocolIEs/{id:139, value}/gTPTunnel/transportLayerAddress/value",
			&key_gtp_te_addr);

	return js_gtp_te_addr == NULL ? NULL : json_object_get_string(js_gtp_te_addr);
}

const char *ngap_get_pdu_gtp_te_id(json_object *js_pdu_sess_elem)
{
	key_list_t key_gtp_te_id = {0,};
	json_object *js_gtp_te_id = search_json_object_ex(js_pdu_sess_elem,
			"/pDUSessionResourceSetupRequestTransfer/containing/protocolIEs/{id:139, value}/gTPTunnel/gTP-TEID",
			&key_gtp_te_id);

	return js_gtp_te_id == NULL ? NULL : json_object_get_string(js_gtp_te_id);
}

const char *ngap_get_pdu_sess_type(json_object *js_pdu_sess_elem)
{
	key_list_t key_pdu_sess_type = {0,};
	json_object *js_pdu_sess_type = search_json_object_ex(js_pdu_sess_elem,
			"/pDUSessionResourceSetupRequestTransfer/containing/protocolIEs/{id:134, value}",
			&key_pdu_sess_type);

	return js_pdu_sess_type == NULL ? NULL : json_object_get_string(js_pdu_sess_type);
}

json_object *ngap_get_pdu_qos_flow_ids(json_object *js_pdu_sess_elem)
{
	key_list_t key_pdu_qos_flow_ids = {0,};
	json_object *js_pdu_qos_flow_ids = search_json_object_ex(js_pdu_sess_elem,
			"/pDUSessionResourceSetupRequestTransfer/containing/protocolIEs/{id:136, value}",
			&key_pdu_qos_flow_ids);

	return js_pdu_qos_flow_ids;
}

