#include <nwucp.h>

extern main_ctx_t *MAIN_CTX;
extern char mySysName[COMM_MAX_NAME_LEN];
extern char myAppName[COMM_MAX_NAME_LEN];

void NWUCP_TRACE(ue_ctx_t *ue_ctx, trace_type_t direction, json_object *js_ngap_pdu, const char *nas_msg)
{
	if (ue_ctx == NULL) {
		return;
	}
	if (direction < DIR_UE_TO_ME || direction > DIR_ME_TO_AMF) {
		return;
	}
	if (msgtrclib_traceOk(TRC_CHECK_START, 0, ue_ctx->ctx_info.ue_id, NULL) == 0) {
		return;
	}

	const char *PROC_STR = NULL;
	if (js_ngap_pdu) {
		key_list_t key_list = {0,};
		memset(&key_list, 0x00, sizeof(key_list_t));
		PROC_STR = NGAP_PROC_C_STR(json_object_get_int(search_json_object_ex(js_ngap_pdu, "/*/procedureCode", &key_list)));
	} else {
		PROC_STR = "N1_Message";
	}

	TraceMsgHead head;
	memset(&head, 0x00, sizeof(TraceMsgHead));

	char trace_buff[65535] = {0,};

	TRACE_MAKE_MSG_HEAD(ue_ctx, direction, PROC_STR, &head, trace_buff);
	TRACE_MAKE_MSG_BODY(trace_buff, js_ngap_pdu != NULL ? JS_PRINT_PRETTY(js_ngap_pdu) : nas_msg);

	msgtrclib_sendTraceMsg(&head, trace_buff);
}

int TRACE_MAKE_MSG_HEAD(ue_ctx_t *ue_ctx, int direction, const char *PROC_STR, TraceMsgHead *head, char *body)
{
    /* set TraceMsgHead */
	switch (direction)
	{
		case DIR_UE_TO_ME:
			head->direction = DIR_RECEIVE;
			head->originSysType = TRACE_NODE_UE;
			head->destSysType = TRACE_NODE_CP;
			sprintf(head->opCode, "NAS_RECV");
			break;
		case DIR_ME_TO_UE:
			head->direction = DIR_SEND;
			head->originSysType = TRACE_NODE_CP;
			head->destSysType = TRACE_NODE_UE;
			sprintf(head->opCode, "NAS_SEND");
			break;
		case DIR_AMF_TO_ME:
			head->direction = DIR_RECEIVE;
			head->originSysType = TRACE_NODE_AMF;
			head->destSysType = TRACE_NODE_CP;
			sprintf(head->opCode, "NGAP_RECV");
			break;
		case DIR_ME_TO_AMF:
			head->direction = DIR_SEND;
			head->originSysType = TRACE_NODE_CP;
			head->destSysType = TRACE_NODE_AMF;
			sprintf(head->opCode, "NGAP_SEND");
			break;
	}
    sprintf(head->primaryKey, ue_ctx->ctx_info.ue_id);

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
    len += sprintf(&body[len], " TRACE KEY          : %s\n", ue_ctx->ctx_info.ue_id);
    len += sprintf(&body[len], " TRACE BLOCK        : %s\n", myAppName);
    len += sprintf(&body[len], "---------------------------------------------------------------\n");
	switch (direction)
	{
		case DIR_UE_TO_ME:
			if (ue_ctx->sock_ctx != NULL) {
				len += sprintf(&body[len], " %-19s  = UE (%s:%d) -> NWUCP\n", "DIRECT", ue_ctx->sock_ctx->client_ipaddr, ue_ctx->sock_ctx->client_port);
			} else {
				len += sprintf(&body[len], " FATAL, SOCK_CTX not exist!\n");
			}
			len += sprintf(&body[len], " %-19s  = %s\n", "MSG_TYPE", "NAS_RECV");
			break;
		case DIR_ME_TO_UE:
			if (ue_ctx->sock_ctx != NULL) {
				len += sprintf(&body[len], " %-19s  = NWUCP -> UE (%s:%d)\n", "DIRECT", ue_ctx->sock_ctx->client_ipaddr, ue_ctx->sock_ctx->client_port);
			} else {
				len += sprintf(&body[len], " FATAL, SOCK_CTX not exist!\n");
			}
			len += sprintf(&body[len], " %-19s  = %s\n", "MSG_TYPE", "NAS_SEND");
			break;
		case DIR_AMF_TO_ME:
			len += sprintf(&body[len], " %-19s  = AMF HOST (%s) -> NWUCP\n", "DIRECT", ue_ctx->sctp_tag.hostname);
			len += sprintf(&body[len], " %-19s  = %s\n", "MSG_TYPE", "NGAP_RECV");
			break;
		case DIR_ME_TO_AMF:
			len += sprintf(&body[len], " %-19s  = NWUCP -> AMF HOST (%s)\n", "DIRECT", ue_ctx->amf_tag.amf_host);
			len += sprintf(&body[len], " %-19s  = %s\n", "MSG_TYPE", "NGAP_SEND");
			break;
	}
	len += sprintf(&body[len], " %-19s  = %s\n", "PROC_CODE", PROC_STR);
    len += sprintf(&body[len], "---------------------------------------------------------------\n");

    return len;
}

size_t TRACE_MAKE_MSG_BODY(char *body, const char *raw_msg)
{
	size_t len = strlen(body);
	len += sprintf(body + len, "%s\n", raw_msg);

	return len;
}
