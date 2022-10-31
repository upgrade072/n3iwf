#include <eapp.h>

extern main_ctx_t *MAIN_CTX;

static int IO_WORKER_DISTR;

void udp_sock_read_callback(int fd, short event, void *arg)
{ 
	recv_buf_t *recv_buf = NULL;

	socklen_t fl = sizeof(struct sockaddr_in);

	if ((recv_buf = get_recv_buf(&MAIN_CTX->udp_buff)) == NULL) {
		ERRLOG(LLE, FL, "{dbg} oops~ %s() can't get recv_buf!\n", __func__);
		goto USRC_ERR;
	}

	if ((recv_buf->size = 
			recvfromto(fd, recv_buf->buffer, MAIN_CTX->udp_buff.each_size, 0, 
			(struct sockaddr *)&recv_buf->from_addr, &fl, NULL, NULL)) < 0) {
		ERRLOG(LLE, FL, "{dbg} oops~ %s() err (%d:%s)\n", __func__, errno, strerror(errno));
		goto USRC_ERR;
	}

	/* relay to io_worker */
	IO_WORKER_DISTR = (IO_WORKER_DISTR + 1) % MAIN_CTX->IO_WORKERS.worker_num;
	worker_ctx_t *io_worker = &MAIN_CTX->IO_WORKERS.workers[IO_WORKER_DISTR];
	event_base_once(io_worker->evbase_thrd, -1, EV_TIMEOUT, handle_udp_request, recv_buf, NULL);
	return;

USRC_ERR:
	if (recv_buf != NULL) {
		release_recv_buf(&MAIN_CTX->udp_buff, recv_buf);
	}
}

void handle_pkt_ntohs(n3iwf_msg_t *n3iwf_msg)
{
    n3iwf_msg->ctx_info.up_id = ntohs(n3iwf_msg->ctx_info.up_id);
    n3iwf_msg->ctx_info.cp_id = ntohs(n3iwf_msg->ctx_info.cp_id);
    n3iwf_msg->payload_len = ntohs(n3iwf_msg->payload_len);
}

void handle_pkg_htons(n3iwf_msg_t *n3iwf_msg)
{
    n3iwf_msg->ctx_info.up_id = htons(n3iwf_msg->ctx_info.up_id);
    n3iwf_msg->ctx_info.cp_id = htons(n3iwf_msg->ctx_info.cp_id);
    n3iwf_msg->payload_len = htons(n3iwf_msg->payload_len);
}

void create_ike_tag(ike_tag_t *ike_tag, struct sockaddr_in *from_addr)
{
	inet_ntop(AF_INET, &from_addr->sin_addr, ike_tag->up_from_addr, INET_ADDRSTRLEN);
	ike_tag->up_from_port = ntohs(from_addr->sin_port);
}

const char *n3_msg_code_str(int msg_code)
{   
    switch(msg_code) {
        case N3_MSG_UNSET:
            return "N3_MSG_UNSET";
        case N3_IKE_AUTH_REQ:
            return "IKE_AUTH_REQ";
        case N3_IKE_AUTH_RES:
            return "IKE_AUTH_RES";
		case N3_IKE_IPSEC_NOTI:
			return "IKE_IPSEC_NOTI";
        case N3_IKE_INFORM_REQ:
            return "IKE_INFORM_REQ";
        case N3_IKE_INFORM_RES:
            return "IKE_INFORM_RES";
        case N3_CREATE_CHILD_SA_REQ:
            return "CREATE_CHILD_SA_REQ";
        case N3_CREATE_CHILD_SA_RES:
            return "CREATE_CHILD_SA_RES";
        default:
            return "UNKNOWN";
    }
}
const char *n3_res_code_str(int res_code)
{   
    switch (res_code) {
        case N3_RES_UNSET:
            return "N3_RES_UNSET";
        case N3_EAP_INIT:
            return "EAP_INIT";
        case N3_EAP_REQUEST:
            return "EAP_REQUEST";
		case N3_EAP_RESPONSE:
            return "EAP_RESPONSE";
        case N3_EAP_SUCCESS:
            return "EAP_SUCCESS";
        case N3_EAP_FAILURE:
            return "EAP_FAILURE";
		case N3_IPSEC_CREATE_SUCCESS:
			return "IPSEC_CREATE_SUCCESS";
		case N3_IPSEC_CREATE_FAILURE:
			return "IPSEC_CREATE_FAILURE";
		case N3_PDU_CREATE_REQUEST:
			return "PDU_CREATE_REQUEST";
		case N3_PDU_CREATE_SUCCESS:
			return "PDU_CREATE_SUCCESS";
		case N3_PDU_CREATE_FAILURE:
			return "PDU_CREATE_FAILURE";
		case N3_PDU_DELETE_REQUEST:
			return "PDU_DELETE_REQUEST";
		case N3_PDU_DELETE_SUCCESS:
			return "PDU_DELETE_SUCCESS";
		case N3_PDU_DELETE_FAILURE:
			return "PDU_DELETE_FAILURE";
		case N3_PDU_UPDATE_REQUEST:
			return "PDU_UPDATE_REQUEST";
		case N3_PDU_UPDATE_SUCCESS:
			return "PDU_UPDATE_SUCCESS";
		case N3_PDU_UPDATE_FAILURE:
			return "PDU_UPDATE_FAILURE";
		case N3_IPSEC_DELETE_UE_CTX:
			return "IPSEC_DELETE_UE_CTX";
		case N3_IPSEC_UE_DISCONNECT:
			return "IPSEC_UE_DISCONNECT";
        default:
            return "UNKNOWN";
    }
}

const char *eap5g_msg_id_str(int msg_id)
{   
    switch (msg_id) {
        case NAS_5GS_START:
            return "nas_5gs_start";
        case NAS_5GS_NAS:
            return "nas_5gs_nas";
        case NAS_5GS_NOTI:
            return "nas_5gs_noti";
        case NAS_5GS_STOP:
            return "nas_5gs_stop";
        default:
            return "unknown";
    }
}
