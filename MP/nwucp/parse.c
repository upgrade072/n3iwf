#include <nwucp.h>
extern main_ctx_t *MAIN_CTX;

json_object *ngap_get_allowed_nssai(json_object *js_ngap_pdu)
{
	key_list_t key_allowed_nssai = {0,};
	json_object *js_allowed_nssai = 
		search_json_object_ex(js_ngap_pdu, "/*/value/protocolIEs/{id:0, value}", &key_allowed_nssai);

	return js_allowed_nssai;
}

const char *ngap_get_amf_name(json_object *js_ngap_pdu)
{
	key_list_t key_amf_name = {0,};
	json_object *js_amf_name = 
		search_json_object_ex(js_ngap_pdu, "/*/value/protocolIEs/{id:1, value}", &key_amf_name);

	return js_amf_name == NULL ? NULL : json_object_get_string(js_amf_name);
}

int64_t ngap_get_amf_ue_ngap_id(json_object *js_ngap_pdu)
{
	key_list_t key_amf_ue_ngap_id = {0,};
	json_object *js_amf_ue_ngap_id = 
		search_json_object_ex(js_ngap_pdu, "/*/value/protocolIEs/{id:10, value}", &key_amf_ue_ngap_id);

	return js_amf_ue_ngap_id == NULL ? -1 : json_object_get_int64(js_amf_ue_ngap_id);
}

const char *ngap_get_ctx_rel_cause(json_object *js_ngap_pdu)
{
	key_list_t key_rel_cause = {0,};
	json_object *js_rel_cause = 
		search_json_object_ex(js_ngap_pdu, "/*/value/protocolIEs/{id:15, value}/*/", &key_rel_cause);

	return js_rel_cause == NULL ? NULL : json_object_get_string(js_rel_cause);
}

json_object *ngap_get_guami(json_object *js_ngap_pdu)
{
	key_list_t key_guami = {0,};
	json_object *js_guami = 
		search_json_object_ex(js_ngap_pdu, "/*/value/protocolIEs/{id:28, value}", &key_guami);

	return js_guami;
}

const char *ngap_get_masked_imeisv(json_object *js_ngap_pdu)
{
	key_list_t key_masked_imeisv = {0,};
	json_object *js_masked_imeisv = 
		search_json_object_ex(js_ngap_pdu, "/*/value/protocolIEs/{id:34, value}", &key_masked_imeisv);

	return js_masked_imeisv == NULL ? NULL : json_object_get_string(js_masked_imeisv);
}

const char *ngap_get_nas_pdu(json_object *js_ngap_pdu)
{
	key_list_t key_nas_pdu = {0,};
	json_object *js_nas_pdu = 
		search_json_object_ex(js_ngap_pdu, "/*/value/protocolIEs/{id:38, value}", &key_nas_pdu);

	return js_nas_pdu == NULL ? NULL : json_object_get_string(js_nas_pdu);
}

json_object *ngap_get_plmn_support_list(json_object *js_ngap_pdu)
{
	key_list_t key_plmn_support_list = {0,};
	json_object *js_plmn_support_list = 
		search_json_object_ex(js_ngap_pdu, "/*/value/protocolIEs/{id:80, value}", &key_plmn_support_list);

	return js_plmn_support_list;
}

int64_t ngap_get_ran_ue_ngap_id(json_object *js_ngap_pdu)
{
	key_list_t key_ran_ue_ngap_id = {0,};
	json_object *js_ran_ue_ngap_id = 
		search_json_object_ex(js_ngap_pdu, "/*/value/protocolIEs/{id:85, value}", &key_ran_ue_ngap_id);

	return js_ran_ue_ngap_id == NULL ? -1 : json_object_get_int64(js_ran_ue_ngap_id);
}

int64_t ngap_get_relative_amf_capacity(json_object *js_ngap_pdu)
{
	key_list_t key_relative_amf_capacity = {0,};
	json_object *js_relative_amf_capacity = 
		search_json_object_ex(js_ngap_pdu, "/*/value/protocolIEs/{id:86, value}", &key_relative_amf_capacity);

	return js_relative_amf_capacity == NULL ? -1 : json_object_get_int64(js_relative_amf_capacity);
}

const char *ngap_get_security_key(json_object *js_ngap_pdu)
{
	key_list_t key_security_key = {0,};
	json_object *js_security_key = 
		search_json_object_ex(js_ngap_pdu, "/*/value/protocolIEs/{id:94, value}", &key_security_key);

	return js_security_key == NULL ? NULL : json_object_get_string(js_security_key);
}

json_object *ngap_get_served_gaumi_list(json_object *js_ngap_pdu)
{
	key_list_t key_served_guami_list = {0,};
	json_object *js_served_guami_list = 
		search_json_object_ex(js_ngap_pdu, "/*/value/protocolIEs/{id:96, value}", &key_served_guami_list);

	return js_served_guami_list;
}

int64_t ngap_get_ctx_rel_amf_ue_ngap_id(json_object *js_ngap_pdu)
{
	key_list_t key_amf_ue_ngap_id = {0,};
	json_object *js_amf_ue_ngap_id = 
		search_json_object_ex(js_ngap_pdu, "/*/value/protocolIEs/{id:114, value}/uE-NGAP-ID-pair/aMF-UE-NGAP-ID", &key_amf_ue_ngap_id);

	return js_amf_ue_ngap_id == NULL ? -1 : json_object_get_int64(js_amf_ue_ngap_id);
}

int64_t ngap_get_ctx_rel_ran_ue_ngap_id(json_object *js_ngap_pdu)
{
	key_list_t key_ran_ue_ngap_id = {0,};
	json_object *js_ran_ue_ngap_id = 
		search_json_object_ex(js_ngap_pdu, "/*/value/protocolIEs/{id:114, value}/uE-NGAP-ID-pair/rAN-UE-NGAP-ID", &key_ran_ue_ngap_id);

	return js_ran_ue_ngap_id == NULL ? -1 : json_object_get_int64(js_ran_ue_ngap_id);
}

json_object *ngap_get_ue_security_capabilities(json_object *js_ngap_pdu)
{
	key_list_t key_ue_security_capabilities = {0,};
	json_object *js_ue_security_capabilities = 
		search_json_object_ex(js_ngap_pdu, "/*/value/protocolIEs/{id:119, value}", &key_ue_security_capabilities);

	return js_ue_security_capabilities;
}

json_object *ngap_get_unavailable_guami_list(json_object *js_ngap_pdu)
{
	key_list_t key_unavailable_guami_list = {0,};
	json_object *js_unavailable_guami_list = 
		search_json_object_ex(js_ngap_pdu, "/*/value/protocolIEs/{id:120, value}", &key_unavailable_guami_list);

	return js_unavailable_guami_list;
}

int64_t ngap_get_pdu_sess_ambr_dl(json_object *js_pdu_sess_elem)
{
	key_list_t key_pdu_ambr_dl = {0,};
	json_object *js_pdu_ambr_dl = 
		search_json_object_ex(js_pdu_sess_elem, 
				"/pDUSessionResourceSetupRequestTransfer/containing/protocolIEs/{id:130, value}/pDUSessionAggregateMaximumBitRateDL", 
				&key_pdu_ambr_dl);

	return js_pdu_ambr_dl == NULL ? -1 : json_object_get_int64(js_pdu_ambr_dl);
}

int64_t ngap_get_pdu_sess_ambr_ul(json_object *js_pdu_sess_elem)
{
	key_list_t key_pdu_ambr_ul = {0,};
	json_object *js_pdu_ambr_ul = 
		search_json_object_ex(js_pdu_sess_elem, 
			"/pDUSessionResourceSetupRequestTransfer/containing/protocolIEs/{id:130, value}/pDUSessionAggregateMaximumBitRateUL", 
			&key_pdu_ambr_ul);

	return js_pdu_ambr_ul == NULL ? -1 : json_object_get_int64(js_pdu_ambr_ul);
}

const char *ngap_get_pdu_sess_type(json_object *js_pdu_sess_elem)
{
	key_list_t key_pdu_sess_type = {0,};
	json_object *js_pdu_sess_type = 
		search_json_object_ex(js_pdu_sess_elem,
			"/pDUSessionResourceSetupRequestTransfer/containing/protocolIEs/{id:134, value}",
			&key_pdu_sess_type);

	return js_pdu_sess_type == NULL ? NULL : json_object_get_string(js_pdu_sess_type);
}

json_object *ngap_get_pdu_qos_flow_ids(json_object *js_pdu_sess_elem)
{
	key_list_t key_pdu_qos_flow_ids = {0,};
	json_object *js_pdu_qos_flow_ids = 
		search_json_object_ex(js_pdu_sess_elem,
			"/pDUSessionResourceSetupRequestTransfer/containing/protocolIEs/{id:136, value}",
			&key_pdu_qos_flow_ids);

	return js_pdu_qos_flow_ids;
}

const char *ngap_get_pdu_gtp_te_addr(json_object *js_pdu_sess_elem)
{
	key_list_t key_gtp_te_addr = {0,};
	json_object *js_gtp_te_addr = 
		search_json_object_ex(js_pdu_sess_elem,
			"/pDUSessionResourceSetupRequestTransfer/containing/protocolIEs/{id:139, value}/gTPTunnel/transportLayerAddress/value",
			&key_gtp_te_addr);

	return js_gtp_te_addr == NULL ? NULL : json_object_get_string(js_gtp_te_addr);
}

const char *ngap_get_pdu_gtp_te_id(json_object *js_pdu_sess_elem)
{
	key_list_t key_gtp_te_id = {0,};
	json_object *js_gtp_te_id = 
		search_json_object_ex(js_pdu_sess_elem,
			"/pDUSessionResourceSetupRequestTransfer/containing/protocolIEs/{id:139, value}/gTPTunnel/gTP-TEID",
			&key_gtp_te_id);

	return js_gtp_te_id == NULL ? NULL : json_object_get_string(js_gtp_te_id);
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

const char *ngap_get_pdu_sess_rel_cause(json_object *js_pdu_sess_elem)
{
	key_list_t key_pdu_sess_rel_cause = {0,};
	json_object *js_pdu_sess_rel_cause = 
		search_json_object_ex(js_pdu_sess_elem, 
				"/pDUSessionResourceReleaseCommandTransfer/containing/cause/*/", 
				&key_pdu_sess_rel_cause);

	return js_pdu_sess_rel_cause == NULL ? NULL : json_object_get_string(js_pdu_sess_rel_cause);
}
