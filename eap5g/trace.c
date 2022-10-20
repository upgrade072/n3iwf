#include <eapp.h>

extern main_ctx_t *MAIN_CTX;
extern char mySysName[COMM_MAX_NAME_LEN];
extern char myAppName[COMM_MAX_NAME_LEN];

void IKE_TRACE(n3iwf_msg_t *n3iwf_msg, ike_msg_t *ike_msg, int direction)
{
	if (direction != DIR_RECEIVE && direction != DIR_SEND) {
		return;
	}
	if (msgtrclib_traceOk(TRC_CHECK_START, 0, n3iwf_msg->ctx_info.ue_id, NULL) == 0) {
		return;
	}

	TraceMsgHead head;
	memset(&head, 0x00, sizeof(TraceMsgHead));

	char trace_buff[65535] = {0,};

	TRACE_MAKE_MSG_HEAD(n3iwf_msg, ike_msg, direction, &head, trace_buff);
	TRACE_MAKE_MSG_BODY(ike_msg, trace_buff);

	msgtrclib_sendTraceMsg(&head, trace_buff);
}

int TRACE_MAKE_MSG_HEAD(n3iwf_msg_t *n3iwf_msg, ike_msg_t *ike_msg, int direction, TraceMsgHead *head, char *body)
{   
    /* set TraceMsgHead */
    head->direction = direction;
    if (direction == DIR_RECEIVE) {
        head->originSysType = TRACE_NODE_UP;
        head->destSysType = TRACE_NODE_CP;
    } else { 
        head->originSysType = TRACE_NODE_CP;
        head->destSysType = TRACE_NODE_UP;
    }
    sprintf(head->opCode, "%s", n3_msg_code_str(n3iwf_msg->msg_code));
    sprintf(head->primaryKey, n3iwf_msg->ctx_info.ue_id);

	struct timespec session_time = {0,};
	clock_gettime(CLOCK_REALTIME, &session_time);
    
    /* set body */
    struct tm *cur_tm = localtime(&session_time.tv_sec);
    int tv_nsec = session_time.tv_nsec;
    
    int len = 0;
    len  = sprintf(&body[len], "===============================================================\n");
    len += sprintf(&body[len], " TRACE TIME         : %d-%02d-%02d %02d:%02d:%02d.%d\n",
            1900 + cur_tm->tm_year,
            1 + cur_tm->tm_mon,
            cur_tm->tm_mday,
            cur_tm->tm_hour,
            cur_tm->tm_min,
            cur_tm->tm_sec, 
            tv_nsec);
    len += sprintf(&body[len], " TRACE SYSTEM       : %s\n", mySysName);
    len += sprintf(&body[len], " TRACE KEY          : %s\n", n3iwf_msg->ctx_info.ue_id);
    len += sprintf(&body[len], " TRACE BLOCK        : %s\n", myAppName);
    len += sprintf(&body[len], "---------------------------------------------------------------\n");
    len += sprintf(&body[len], " CP ID                = %d\n", n3iwf_msg->ctx_info.cp_id);
    len += sprintf(&body[len], " UP ID                = %d\n", n3iwf_msg->ctx_info.up_id);
	if (direction == DIR_RECEIVE) {
		len += sprintf(&body[len], " %-19s  = %s:%d -> EAP5G\n", "DIRECT", ike_msg->ike_tag.up_from_addr, ike_msg->ike_tag.up_from_port);
	} else{
		len += sprintf(&body[len], " %-19s  = EAP5G -> %s:%d\n", "DIRECT", ike_msg->ike_tag.up_from_addr, ike_msg->ike_tag.up_from_port);
	}
	len += sprintf(&body[len], " %-19s  = (%.2d) %s\n", "MSG_CODE", n3iwf_msg->msg_code, n3_msg_code_str(n3iwf_msg->msg_code));
	len += sprintf(&body[len], " %-19s  = (%.2d) %s\n", "RES_CODE", n3iwf_msg->res_code, n3_res_code_str(n3iwf_msg->res_code));
    len += sprintf(&body[len], "---------------------------------------------------------------\n");

    return len;
}  

size_t TRACE_MAKE_MSG_BODY(ike_msg_t *ike_msg, char *body)
{
	size_t len = strlen(body);

	len += sprintf(body + len, " IKE TAG\n");
	if (strlen(ike_msg->ike_tag.up_from_addr)) {
		len += sprintf(body + len, "  up from addr       : %s\n", ike_msg->ike_tag.up_from_addr);
		len += sprintf(body + len, "  up from port       : %d\n", ike_msg->ike_tag.up_from_port);
	}
	if (strlen(ike_msg->ike_tag.ue_from_addr)) {
		len += sprintf(body + len, "  ue from addr       : %s\n", ike_msg->ike_tag.ue_from_addr);
		len += sprintf(body + len, "  ue from port       : %d\n", ike_msg->ike_tag.ue_from_port);
	}
	if (strlen(ike_msg->ike_tag.cp_amf_host)) {
		len += sprintf(body + len, "  cp amf host        : %s\n", ike_msg->ike_tag.ue_from_addr);
	}

	n3_pdu_sess_t *pdu_sess = NULL;
	switch (ike_msg->ike_choice) 
	{
		case choice_unset:
			break;
		case choice_eap_result:
			len += sprintf(body + len, " EAP RESULT\n");
			len += sprintf(body + len, "  tcp server ip      : %s\n", ike_msg->ike_data.eap_result.tcp_server_ip);
			len += sprintf(body + len, "  tcp server port    : %d\n", ike_msg->ike_data.eap_result.tcp_server_port);
			len += sprintf(body + len, "  internal ip        : %s\n", ike_msg->ike_data.eap_result.internal_ip);
			len += sprintf(body + len, "  internal netmask   : %d\n", ike_msg->ike_data.eap_result.internal_netmask);
			len += sprintf(body + len, "  security key str   : H'%s\n", ike_msg->ike_data.eap_result.security_key_str);
			break;
		case choice_pdu_info:
			len += sprintf(body + len, " PDU INFO\n");
			len += sprintf(body + len, "  pdu num            : (%d)\n", ike_msg->ike_data.pdu_info.pdu_num);
			for (int i = 0; i < ike_msg->ike_data.pdu_info.pdu_num && i < MAX_N3_PDU_NUM; i++) {
				pdu_sess = &ike_msg->ike_data.pdu_info.pdu_sessions[i];
				len += sprintf(body + len, "  session id         : %d\n", pdu_sess->session_id);
				len += sprintf(body + len, "   sess ambr dl      : %d\n", pdu_sess->pdu_sess_ambr_dl);
				len += sprintf(body + len, "   sess ambr ul      : %d\n", pdu_sess->pdu_sess_ambr_ul);
				len += sprintf(body + len, "   gtp te addr       : %s\n", pdu_sess->gtp_te_addr);
				len += sprintf(body + len, "   gtp te id         : H'%s\n", pdu_sess->gtp_te_id);
				len += sprintf(body + len, "   address family    : %s\n", pdu_sess->address_family == AF_INET ? "AF_INET" : pdu_sess->address_family == AF_INET6 ? "AF_INET6" : "");
				len += sprintf(body + len, "   qos flow id num   : (%d)\n", pdu_sess->qos_flow_id_num);
				if (pdu_sess->qos_flow_id_num) {
					len += sprintf(body + len, "    qos flow id      : ");
					for (int k = 0; k < pdu_sess->qos_flow_id_num && k < MAX_N3_QOS_FLOW_IDS; k++) {
						len += sprintf(body + len, "[%d] ", pdu_sess->qos_flow_id[k]);
					}
					len += sprintf(body + len, "\n");
				}
			}
			break;
		default:
			break;
	}

	if (ike_msg->eap_5g.eap_code) {
		len += sprintf(body + len, " EAP 5G\n");
		len += sprintf(body + len, "  eap code           : %s\n", get_eap_code_str(ike_msg->eap_5g.eap_code));
		len += sprintf(body + len, "  eap id             : %d\n", ike_msg->eap_5g.eap_id);
		len += sprintf(body + len, "  msg id             : %s\n", get_nas5gs_msgid_str(ike_msg->eap_5g.msg_id));
	}

	if (ike_msg->eap_5g.an_param.set > 0) {
		len += sprintf(body + len, " AN PARAMS\n");
		if (ike_msg->eap_5g.an_param.guami.set > 0) {
			len += sprintf(body + len, "  guami\n");
			len += sprintf(body + len, "   plmn id bcd       : H'%s\n", ike_msg->eap_5g.an_param.guami.plmn_id_bcd);
			len += sprintf(body + len, "   amf id            : H'%s\n", ike_msg->eap_5g.an_param.guami.amf_id_str);
		}
		if (ike_msg->eap_5g.an_param.plmn_id.set > 0) {
			len += sprintf(body + len, "  plmn_id\n");
			len += sprintf(body + len, "   plmn_id bcd       : H'%s\n", ike_msg->eap_5g.an_param.plmn_id.plmn_id_bcd);
		}
		if (ike_msg->eap_5g.an_param.nssai.set_num > 0) {
			len += sprintf(body + len, "  nsaai\n");
			for (int i = 0; i < ike_msg->eap_5g.an_param.nssai.set_num && i < MAX_AN_NSSAI_NUM; i++) {
				len += sprintf(body + len, "   set id            : (%d)\n", i);
				len += sprintf(body + len, "    sst              : H'%02X\n", ike_msg->eap_5g.an_param.nssai.sst[i]);
				len += sprintf(body + len, "    sd               : H'%s\n", ike_msg->eap_5g.an_param.nssai.sd_str[i]);
			}
		}
		if (ike_msg->eap_5g.an_param.cause.set > 0) {
			len += sprintf(body + len, "  cause\n");
			len += sprintf(body + len, "   cause             : %s\n", ike_msg->eap_5g.an_param.cause.cause_str);
		}
	}

	if (strlen(ike_msg->eap_5g.nas_str)) {
		len += sprintf(body + len, " NAS PDU\n");
		len += sprintf(body + len, "  nas str            : H'%s\n", ike_msg->eap_5g.nas_str);
	}

	return len;
}
