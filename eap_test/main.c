#include <libutil.h>
#include <libudp.h>
#include <n3iwf_comm.h>
#include <eap.h>

void handle_udp_ntohs(n3iwf_msg_t *n3iwf_msg)
{
	n3iwf_msg->ctx_info.up_id = ntohs(n3iwf_msg->ctx_info.up_id);
	n3iwf_msg->ctx_info.cp_id = ntohs(n3iwf_msg->ctx_info.cp_id);
	n3iwf_msg->payload_len = ntohs(n3iwf_msg->payload_len);
}

void handle_udp_htons(n3iwf_msg_t *n3iwf_msg)
{
	n3iwf_msg->ctx_info.up_id = ntohs(n3iwf_msg->ctx_info.up_id);
	n3iwf_msg->ctx_info.cp_id = ntohs(n3iwf_msg->ctx_info.cp_id);
	n3iwf_msg->payload_len = ntohs(n3iwf_msg->payload_len);
}

const char *n3_msg_code_str(int msg_code)
{
	switch(msg_code) {
		case N3_IKE_AUTH_INIT:
			return "ike_auth_init";
		case N3_IKE_AUTH_REQ:
			return "ike_auth_req";
		case N3_IKE_AUTH_RES:
			return "ike_auth_res";
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
		case N3_EAP_INIT:
			return "eap_init";
		case N3_EAP_REQUEST:
			return "eap_request";
		case N3_EAP_SUCCESS:
			return "eap_success";
		case N3_EAP_FAILURE:
			return "eap_failure";
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

void n3iwf_send(n3iwf_msg_t *n3iwf_msg, const char *prefix, int sock_fd, struct sockaddr_in *to_addr)
{
	fprintf(stderr, "--> [%s] [%s] [%s] ", prefix,
			n3_msg_code_str(n3iwf_msg->msg_code),
			n3_res_code_str(n3iwf_msg->res_code));
	fprintf(stderr, "(up_id=%d cp_id=%d) ", 
			n3iwf_msg->ctx_info.up_id,
			n3iwf_msg->ctx_info.cp_id);

	if (n3iwf_msg->msg_code != N3_IKE_AUTH_INIT) {
		eap_packet_raw_t *eap = (eap_packet_raw_t *)n3iwf_msg->payload;
		fprintf(stderr, "eap_id=(%d) ", eap->id);

		uint8_t msg_id = eap->data[8];
		fprintf(stderr, "nas_code=[%s] ", eap5g_msg_id_str(msg_id));
	}

	char payload[10240] = {0,};
	if (n3iwf_msg->msg_code == N3_IKE_AUTH_INIT) {
		sprintf(payload, "%s", n3iwf_msg->payload);
	} else {
		mem_to_hex((unsigned char *)n3iwf_msg->payload, n3iwf_msg->payload_len, payload); 
	}

	fprintf(stderr, "payload len=(%d) [%.20s...]\n", n3iwf_msg->payload_len, payload); 

	handle_udp_htons(n3iwf_msg);

	int res = sendfromto(sock_fd, n3iwf_msg, N3IWF_MSG_SIZE(n3iwf_msg), 0, NULL, 0, (struct sockaddr *)to_addr, sizeof(struct sockaddr_in));

	if (res < 0) {
		fprintf(stderr, "err (%d:%s)\n", errno, strerror(errno));
	}
}

void n3iwf_recv(n3iwf_msg_t *n3iwf_msg, size_t buff_size, int sock_fd, const char *prefix, struct sockaddr_in *fr_addr)
{
	socklen_t fr_size = 0;

	int res = recvfromto(sock_fd, n3iwf_msg, buff_size, 0, (struct sockaddr *)fr_addr, &fr_size, NULL, NULL);

	if (res < 0) {
		fprintf(stderr, "%s() err (%d:%s)\n", __func__, errno, strerror(errno));
		return;
	}

	handle_udp_ntohs(n3iwf_msg);

	fprintf(stderr, "<-- [%s] [%s] [%s] ", prefix,
			n3_msg_code_str(n3iwf_msg->msg_code),
			n3_res_code_str(n3iwf_msg->res_code));
	fprintf(stderr, "(up_id=%d cp_id=%d) ", 
			n3iwf_msg->ctx_info.up_id,
			n3iwf_msg->ctx_info.cp_id);

	eap_packet_raw_t *eap = (eap_packet_raw_t *)n3iwf_msg->payload;
	fprintf(stderr, "eap_id=(%d) ", eap->id);

	uint8_t msg_id = eap->data[8];
	fprintf(stderr, "nas_code=[%s] ", eap5g_msg_id_str(msg_id));

	char payload[10240] = {0,};
	mem_to_hex((unsigned char *)n3iwf_msg->payload, n3iwf_msg->payload_len, payload); 

	fprintf(stderr, "payload len=(%d) [%.20s...]\n", n3iwf_msg->payload_len, payload); 
}

void n3iwf_load_filedata(char *fname, n3iwf_msg_t *n3iwf_msg)
{
	size_t fsize = 0;
	unsigned char *file_data = file_to_buffer(fname, "r", &fsize);
	memcpy(n3iwf_msg->payload, file_data, fsize);
	free(file_data);
	n3iwf_msg->payload_len = fsize;
}

int main()
{
	int sock_fd = create_udp_sock(20000);

	struct sockaddr_in to_addr, fr_addr;
	memset(&fr_addr, 0x00, sizeof(fr_addr));

	memset(&to_addr, 0x00, sizeof(to_addr));
	to_addr.sin_family = AF_INET;
	to_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	to_addr.sin_port = htons(10000);

	char buffer[10240] = {0,};
	n3iwf_msg_t *n3iwf_msg = (n3iwf_msg_t *)buffer;
	memset(n3iwf_msg, 0x00, sizeof(n3iwf_msg_t));

/* IKE_AUTH Req (UE Id, without AUTH) */
	n3iwf_msg->msg_code = N3_IKE_AUTH_INIT;
	n3iwf_msg->ctx_info.up_id = 1234;
	n3iwf_msg->ctx_info.cp_id = 0;
	n3iwf_msg->payload_len = sprintf(n3iwf_msg->payload, "%s", "test-ueid-123456789@com");

	n3iwf_send(n3iwf_msg, "(UE Id, without Auth)", sock_fd, &to_addr);

/* IKE_AUTH Res (EAP-Req/5G-Start) */
	n3iwf_recv(n3iwf_msg, 10240, sock_fd, "(EAP-5G/NAS-5GS-START)", &fr_addr);

	eap_packet_raw_t *eap = (eap_packet_raw_t *)n3iwf_msg->payload;
	uint8_t eap_id = eap->id;

/* IKE_AUTH Req (EAP-Res/5G-NAS/An-Param/NAS,PDU[Regi Request]) */
	n3iwf_msg->msg_code = N3_IKE_AUTH_REQ;
	n3iwf_load_filedata("../eap5g/sample_eap/5. EAP-Res 5G-NAS (AN-Params, NAS-PDU [Regi Req]).bin", n3iwf_msg);
	eap->id = eap_id;

	n3iwf_send(n3iwf_msg, "(EAP-Res/5G-NAS/An-Param/NAS,PDU[Regi Request])", sock_fd, &to_addr);

/* IKE_AUTH Res (EAP-Req/5G-NAS) */
	n3iwf_recv(n3iwf_msg, 10240, sock_fd, "(EAP-5G/5G-NAS)", &fr_addr);

	eap = (eap_packet_raw_t *)n3iwf_msg->payload;
	eap_id = eap->id;

/* IKE_AUTH Req (EAP-Res/5G-NAS/NAS,PDU[Auth Res [EAP_AKA-Challenge]]) */
	n3iwf_msg->msg_code = N3_IKE_AUTH_REQ;
	n3iwf_load_filedata("../eap5g/sample_eap/8e. EAP-Res 5G-NAS (NAS-PDU [Auth Res [EAP_AKA-Challenge]]).bin", n3iwf_msg);
	eap->id = eap_id;

	n3iwf_send(n3iwf_msg, "(EAP-Res/5G-NAS/NAS,PDU[Auth Res [EAP_AKA-Challenge]])", sock_fd, &to_addr);
}
