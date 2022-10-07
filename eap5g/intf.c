#include <eapp.h>

extern main_ctx_t *MAIN_CTX;

static int IO_WORKER_DISTR;

void udp_sock_read_callback(int fd, short event, void *arg)
{ 
	recv_buf_t *recv_buf = NULL;

	socklen_t fl = sizeof(struct sockaddr_in);

	if ((recv_buf = get_recv_buf(&MAIN_CTX->udp_buff)) == NULL) {
		fprintf(stderr, "{dbg} oops~ %s() can't get recv_buf!\n", __func__);
		goto USRC_ERR;
	}

	if ((recv_buf->size = 
			recvfromto(fd, recv_buf->buffer, MAIN_CTX->udp_buff.each_size, 0, 
			(struct sockaddr *)&recv_buf->from_addr, &fl, NULL, NULL)) < 0) {
		fprintf(stderr, "{dbg} oops~ %s() err (%d:%s)\n", __func__, errno, strerror(errno));
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
            return "n3_msg_unset";
        case N3_IKE_AUTH_REQ:
            return "ike_auth_req";
        case N3_IKE_AUTH_RES:
            return "ike_auth_res";
		case N3_IKE_IPSEC_NOTI:
			return "ike_ipsec_noti";
        case N3_IKE_INFORM_REQ:
            return "ike_inform_req";
        case N3_IKE_INFORM_RES:
            return "ike_inform_res";
        case N3_CREATE_CHILD_SA_REQ:
            return "create_child_sa_req";
        case N3_CREATE_CHILD_SA_RES:
            return "create_child_sa_res";
        default:
            return "unknown";
    }
}
const char *n3_res_code_str(int res_code)
{   
    switch (res_code) {
        case N3_RES_UNSET:
            return "n3_res_unset";
        case N3_EAP_INIT:
            return "eap_init";
        case N3_EAP_REQUEST:
            return "eap_request";
		case N3_EAP_RESPONSE:
            return "eap_response";
        case N3_EAP_SUCCESS:
            return "eap_success";
        case N3_EAP_FAILURE:
            return "eap_failure";
		case N3_IPSEC_CREATE_SUCCESS:
			return "ipsec_create_success";
		case N3_IPSEC_CREATE_FAILURE:
			return "ipsec_create_failure";
		case N3_PDU_CREATE_REQUEST:
			return "pdu_create_request";
		case N3_PDU_CREATE_SUCCESS:
			return "pdu_create_success";
		case N3_PDU_CREATE_FAILURE:
			return "pdu_create_failure";
		case N3_PDU_DELETE_REQUEST:
			return "pdu_delete_request";
		case N3_PDU_DELETE_SUCCESS:
			return "pdu_delete_success";
		case N3_PDU_DELETE_FAILURE:
			return "pdu_delete_failure";
		case N3_PDU_UPDATE_REQUEST:
			return "pdu_update_request";
		case N3_PDU_UPDATE_SUCCESS:
			return "pdu_update_success";
		case N3_PDU_UPDATE_FAILURE:
			return "pdu_update_failure";
		case N3_IPSEC_DELETE_UE_CTX:
			return "ipsec_delete_ue_ctx";
        default:
            return "unknown";
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
