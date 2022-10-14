#include <eap5g.h>

const char eap_code_str[][24] = { 
    "eap_unset",
    "eap_request",
    "eap_response",
    "eap_success",
    "eap_failure" };

const char nas5gs_msgid_str[][24] = {
	"nas_5gs_unset",
	"nas_5gs_start",
	"nas_5gs_nas",
	"nas_5gs_noti",
	"nas_5gs_stop" };

const char *establish_cause_str(establishment_cause_t cause)
{
	switch (cause) {
		case AEC_EMERENGY:
			return "emergency";
		case AEC_HIGH_PRIORITY:
			return "highPriorityAccess";
		case AEC_MO_SIGNALLING:
			return "mo-Signalling";
		case AEC_MO_DATA:
			return "mo-Data";
		case AEC_MPS_PRIORITY:
			return "mps-PriorityAccess";
		case AEC_MCS_PRIORITY:
			return "mcs-PriorityAccess";
		default:
			return "notAvailable";
	}
}

const char *get_eap_code_str(eap_code_t eap_code)
{
	if (eap_code < EAP_UNSET || eap_code >= EAP_MAX_CODES) {
		return "eap_code_invalid";
	} else {
		return eap_code_str[eap_code];
	}
}

const char *get_nas5gs_msgid_str(msgid_type_t msg_id)
{
	if (msg_id < NAS_5GS_UNSET || msg_id >= NAS_5GS_MAX_ID) {
		return "nas_5gs_invalid";
	} else {
		return nas5gs_msgid_str[msg_id];
	}
}

size_t encap_eap_req(eap_relay_t *eap_relay, unsigned char *buffer, size_t buffer_size)
{
	if (eap_relay->msg_id != NAS_5GS_START && strlen(eap_relay->nas_str) == 0) {
		fprintf(stderr, "%s() recv invalid request (nas_str == NULL)!\n", __func__);
		return 0;
	}
	int eap_len = eap_relay->msg_id == NAS_5GS_START ? 14 : 
		(14 + 2 /* NAS PDU len field */ + strlen(eap_relay->nas_str)/2 /* NAS PDU size */);

	eap_packet_raw_t *eap = (eap_packet_raw_t *)buffer;

	eap->code = eap_relay->eap_code;
	eap->id = eap_relay->eap_id;
	uint16_t len = htons(eap_len);
	memcpy(eap->length, &len, sizeof(uint16_t));
	eap->data[0] = EAP_EXPANDED_TYPE;

	/* Vendor ID : 0x28af */
	eap->data[1] = 0x00;
	eap->data[2] = 0x28;
	eap->data[3] = 0xaf;

	/* Vendor Type : 0x03 */
	eap->data[4] = 0x00;
	eap->data[5] = 0x00;
	eap->data[6] = 0x00;
	eap->data[7] = 0x03;

	/* Message ID : (int message_id) */
	eap->data[8] = eap_relay->msg_id;

	/* Spare */
	eap->data[9] = 0x00;

	if (eap_relay->msg_id == NAS_5GS_START) {
		/* EOF */
	} else {
		/* add NAS_PDU */
		size_t nas_len = 0;
		hex_to_mem(eap_relay->nas_str, (char *)&eap->data[12], &nas_len);

		uint16_t nas_pdu_len = ntohs(nas_len);
		memcpy(&eap->data[10], &nas_pdu_len, sizeof(nas_pdu_len));
	}

	fprintf(stderr, "{dbg} %s() send eap=[%s](id:%d)/nas_5gs=[%s]\n", 
			__func__, eap_code_str[eap->code], eap->id, nas5gs_msgid_str[eap_relay->msg_id]);

	return eap_len;
}

size_t encap_eap_result(eap_relay_t *eap_relay, unsigned char *buffer, size_t buffer_size)
{
	int eap_len = 4;

	eap_packet_raw_t *eap = (eap_packet_raw_t *)buffer;

	eap->code = eap_relay->eap_code;
	eap->id = eap_relay->eap_id;
	uint16_t len = htons(eap_len);
	memcpy(eap->length, &len, sizeof(uint16_t));

	uint8_t message_id = eap->data[0] == EAP_EXPANDED_TYPE ? eap->data[8] : 0;
	fprintf(stderr, "{dbg} %s() send eap=[%s]/nas_5gs=[%s]\n", 
			__func__, eap_code_str[eap->code], nas5gs_msgid_str[message_id]);

	return eap_len;
}

void parse_an_guami(unsigned char *params, size_t len, an_guami_t *guami)
{
	uint8_t mcc_2 = (params[0] >> 4) & 0x0f;
	uint8_t mcc_1 = params[0] & 0x0f;
	uint8_t mnc_3 = (params[1] >> 4) & 0x0f;
	uint8_t mcc_3 = params[1] & 0x0f;
	uint8_t mnc_2 = (params[2] >> 4) & 0x0f;
	uint8_t mnc_1 = params[2] & 0x0f;
	uint8_t amf_region_id = params[3];
	uint16_t amf_set_id = (params[4] << 2) | ((params[5] >> 6) & 0x03);
	uint8_t amf_pointer = params[5] & 0x3f;

	guami->set = 1;
	sprintf(guami->mcc, "%X%X%X", mcc_1, mcc_2, mcc_3);
	if (mnc_3 == 0xF) {
	sprintf(guami->mnc, "%X%X%X", 0xF, mnc_1, mnc_2);
	} else {
	sprintf(guami->mnc, "%X%X%X", mnc_1, mnc_2, mnc_3);
	}
	guami->amf_region_id = amf_region_id;
	guami->amf_set_id = amf_set_id;
	guami->amf_pointer = amf_pointer;
	mem_to_hex(&params[3], 3, guami->amf_id_str);

	fprintf(stderr, "%s() mcc=(0x%s), mnc=(0x%s), amf_id=(%s)\n", 
			__func__, guami->mcc, guami->mnc, guami->amf_id_str);

	char mccmnc[7] = {0,};
	sprintf(mccmnc, "%s%s", guami->mcc, guami->mnc);
	print_bcd_str(mccmnc, guami->plmn_id_bcd, 7);
	fprintf(stderr, "%s() guami plmn_id_bcd=(0x%s)\n", __func__, guami->plmn_id_bcd);
}

void parse_an_plmn_id(unsigned char *params, size_t len, an_plmn_id_t *plmn_id)
{
	uint8_t mcc_2 = (params[0] >> 4) & 0x0f;
	uint8_t mcc_1 = params[0] & 0x0f;
	uint8_t mnc_3 = (params[1] >> 4) & 0x0f;
	uint8_t mcc_3 = params[1] & 0x0f;
	uint8_t mnc_2 = (params[2] >> 4) & 0x0f;
	uint8_t mnc_1 = params[2] & 0x0f;

	plmn_id->set = 1;
	sprintf(plmn_id->mcc, "%X%X%X", mcc_1, mcc_2, mcc_3);
	if (mnc_3 == 0xF) {
	sprintf(plmn_id->mnc, "%X%X%X", 0xF, mnc_1, mnc_2);
	} else {
	sprintf(plmn_id->mnc, "%X%X%X", mnc_1, mnc_2, mnc_3);
	}

	fprintf(stderr, "%s() mcc=(0x%s) mnc=(0x%s)\n",
			__func__, plmn_id->mcc, plmn_id->mnc);

	char mccmnc[7] = {0,};
	sprintf(mccmnc, "%s%s", plmn_id->mcc, plmn_id->mnc);
	print_bcd_str(mccmnc, plmn_id->plmn_id_bcd, 7);
	fprintf(stderr, "%s() an plmn_id_bcd=(0x%s)\n", __func__, plmn_id->plmn_id_bcd);
}

void parse_an_aec(unsigned char *params, size_t len, an_cause_t *cause)
{
	uint8_t aec = params[0];

	cause->set = 1;
	cause->ec = aec;
	sprintf(cause->cause_str, "%s", establish_cause_str(aec));
	fprintf(stderr, "%s() aec=(0x%x:%s)\n", __func__, aec, cause->cause_str);
}

void parse_an_nssai(unsigned char *params, size_t total_len, an_nssai_t *nssai)
{
	int remain = total_len;
	int progress = 0;
	nssai->set_num = 0;

	while (remain >= 2 && nssai->set_num < MAX_AN_NSSAI_NUM) {
		uint8_t len = params[progress++];
		if (len != 4) {
			fprintf(stderr, "{dbg} %s() error, we can only parse SST_SSD type!\n", __func__);
			return;
		}
		uint8_t sst = params[progress];
		char sd_str[6+1] = {0,};
		mem_to_hex(&params[progress+1], 3, sd_str);

		nssai->sst[nssai->set_num] = sst;
		sprintf(nssai->sd_str[nssai->set_num], "%s", sd_str);
		fprintf(stderr, "%s() sst=(%d) sd=(%s)\n", __func__, nssai->sst[nssai->set_num], nssai->sd_str[nssai->set_num]);
		nssai->set_num += 1;

		progress += len;
		remain -= (len + 1);
	}
}

void parse_an_params(unsigned char *params, size_t total_len, an_param_t *an_param)
{
	int remain = total_len;
	int progress = 0;
	while (remain >= sizeof(an_param_raw_t)) {
		an_param->set = 1;
		an_param_raw_t *an_param_raw = (an_param_raw_t *)&params[progress];

		int tlv_len = 1 + 1 + an_param_raw->length;
		remain = remain - tlv_len;
		progress = progress + tlv_len;

		fprintf(stderr, "%s() T:%d L:%d V:", __func__, an_param_raw->type, an_param_raw->length);

		switch (an_param_raw->type) {
			case AN_GUAMI:
				fprintf(stderr, "Guami\n");
				parse_an_guami(an_param_raw->value, an_param_raw->length, &an_param->guami);
				break;
			case AN_PLMN_ID:
				fprintf(stderr, "Plmn_ID\n");
				parse_an_plmn_id(an_param_raw->value, an_param_raw->length, &an_param->plmn_id);
				break;
			case AN_NSSAI:
				fprintf(stderr, "Nssai\n");
				parse_an_nssai(an_param_raw->value, an_param_raw->length, &an_param->nssai);
				break;
			case AN_CAUSE:
				fprintf(stderr, "Cause\n");
				parse_an_aec(an_param_raw->value, an_param_raw->length, &an_param->cause);
				break;
			case AN_NID:
				fprintf(stderr, "NID\n");
				break;
			case AN_UE_ID:
				fprintf(stderr, "Ue_ID\n");
				break;
			default:
				fprintf(stderr, "Unknown\n");
				break;
		}
	}
}

void parse_nas_pdu(unsigned char *params, size_t total_len, char *nas_str)
{
	mem_to_hex(params, total_len, nas_str);
	fprintf(stderr, "%s() nas_pdu (len:%ld) [%s]\n", __func__, total_len, nas_str);
}

void decap_eap_res(eap_relay_t *eap_relay, unsigned char *buffer, size_t buffer_size)
{
	if (buffer_size < EAP_HEADER_LEN) {
		fprintf(stderr, "{dbg} %s() with insufficient buffer size!\n", __func__);
		return;
	}

	eap_packet_raw_t *eap = (eap_packet_raw_t *)buffer;

	uint16_t len;
	memcpy(&len, eap->length, sizeof(uint16_t));
	len = ntohs(len);

	uint8_t message_id = eap->data[8];
	if ((eap->code != EAP_RESPONSE) ||
		(len != buffer_size || len < 20) ||
		(eap->data[0] != EAP_EXPANDED_TYPE) ||
		(eap->data[1] != 0x00 || eap->data[2] != 0x28 || eap->data[3] != 0xaf) ||
		(eap->data[4] != 0x00 || eap->data[5] != 0x00 || eap->data[6] != 0x00 || eap->data[7] != 0x03) ||
		(message_id != NAS_5GS_NAS && message_id != NAS_5GS_STOP) ||
		(eap->data[9] != 0x00)) {
		fprintf(stderr, "{dbg} %s() with invalid eap_res_5g_nas!\n", __func__);
		return;
	} else {
		eap_relay->eap_code = eap->code;
		eap_relay->eap_id = eap->id;
		eap_relay->msg_id = message_id;
		fprintf(stderr, "{dbg} %s() recv eap=[%s](id:%d)/nas_5gs=[%s]\n", 
				__func__, eap_code_str[eap->code], eap->id, nas5gs_msgid_str[message_id]);
	}

	if (message_id != NAS_5GS_NAS) {
		return;
	}
	
	/* an parameter */
	uint16_t an_len;
	memcpy(&an_len, &eap->data[10], sizeof(uint16_t)); /* consume 10, 11 */
	an_len = ntohs(an_len);
	unsigned char *an_params = &eap->data[10 + 2];
	uint16_t an_cut_pos = 10 + 2 + an_len;

	/* nas pdu */
	uint16_t nas_len;
	memcpy(&nas_len, &eap->data[an_cut_pos], sizeof(uint16_t));
	nas_len = ntohs(nas_len);
	unsigned char *nas_pdu = &eap->data[an_cut_pos + 2];
	uint16_t nas_cut_pos = an_cut_pos + 2 + nas_len;

	if (len != (EAP_HEADER_LEN + nas_cut_pos)) {
		fprintf(stderr, "{dbg} %s() recv invalid an/nas len!\n", __func__);
		return;
	}
	fprintf(stderr, "{dbg} %s() check len an=(%d) nas=(%d)\n", __func__, an_len, nas_len);

	/* parse an_params */
	parse_an_params(an_params, an_len, &eap_relay->an_param);

	/* parse nas_pdu */
	parse_nas_pdu(nas_pdu, nas_len, eap_relay->nas_str);
}

