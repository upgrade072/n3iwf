#include <stdio.h>
#include <stdint.h>

#define N3_EXPIRE_TM_SEC		3

#pragma pack(push, 1)

typedef enum n3iwf_msgcode_t {
	N3_MSG_UNSET = 0,
	N3_IKE_AUTH_REQ,
	N3_IKE_AUTH_RES,
	N3_IKE_IPSEC_NOTI,
	N3_IKE_INFORM_REQ,
	N3_IKE_INFORM_RES,
	N3_CREATE_CHILD_SA_REQ,
	N3_CREATE_CHILD_SA_RES
} n3iwf_msgcode_t;

typedef enum n3iwf_rescode_t {
	N3_RES_UNSET = 0,
	N3_EAP_INIT,				// n3_eap_init_t
	N3_EAP_REQUEST,				// [eap payload]
	N3_EAP_RESPONSE,			// [eap payload]
	N3_EAP_SUCCESS,				// n3_eap_result_t
	N3_EAP_FAILURE,				// n3_eap_result_t
	N3_IPSEC_CREATE_SUCCESS,	// [no payload]
	N3_IPSEC_CREATE_FAILURE,	// [no payload]
	N3_PDU_CREATE_REQUEST,		// n3_pdu_sess_t
	N3_PDU_CREATE_SUCCESS, 		// n3_pdu_sess_t
	N3_PDU_CREATE_FAILURE,		// n3_pdu_sess_t
	N3_PDU_DELETE_REQUEST,		// n3_pdu_sess_t
	N3_PDU_DELETE_SUCCESS, 		// n3_pdu_sess_t
	N3_PDU_DELETE_FAILURE,		// n3_pdu_sess_t
	N3_PDU_UPDATE_REQUEST,		// n3_pdu_sess_t
	N3_PDU_UPDATE_SUCCESS, 		// n3_pdu_sess_t
	N3_PDU_UPDATE_FAILURE,		// n3_pdu_sess_t
	N3_IPSEC_DELETE_UE_CTX,		// [no payload]
	N3_IPSEC_UE_DISCONNECT, 	// [no payload]
	/* .... */
} n3iwf_rescode_t;

typedef struct ctx_info_t {
	char		ue_id[128];		/* Identification Data (trace key) */
	uint16_t	up_id;
	uint16_t	cp_id;
} ctx_info_t;

typedef struct n3iwf_msg_t {
	uint8_t		msg_code;		/* n3iwf_msgcode_t */
	uint8_t		res_code;		/* n3iwf_rescode_t */
	ctx_info_t	ctx_info;
	uint16_t	payload_len;
	char		payload[1];	
} n3iwf_msg_t;

#define N3IWF_MSG_SIZE(a)	(sizeof(n3iwf_msg_t) - 1 + htons(a->payload_len))

/* 각 상세 payload 정의 */
typedef struct n3_eap_init_t {
	char		ue_from_addr[INET_ADDRSTRLEN];
	uint16_t	ue_from_port;
} n3_eap_init_t;

typedef struct n3_eap_result_t {
	char		eap_payload[4];			/* eap relay (success or fail) */

	char		tcp_server_ip[INET_ADDRSTRLEN];
	uint16_t	tcp_server_port;
	char		internal_ip[INET_ADDRSTRLEN];
	uint8_t		internal_netmask;
	char		security_key_str[64 + 1];
} n3_eap_result_t;

typedef struct n3_pdu_sess_t {
	uint8_t		session_id;
	uint32_t	pdu_sess_ambr_dl;
	uint32_t	pdu_sess_ambr_ul;
	char		gtp_te_addr[INET6_ADDRSTRLEN];
	char		gtp_te_id[8 + 1];
	uint8_t		address_family;			/* AF_INET | AF_INET6 */

#define MAX_N3_QOS_FLOW_IDS		8
	uint8_t		qos_flow_id_num;
	uint8_t		qos_flow_id[MAX_N3_QOS_FLOW_IDS];
} n3_pdu_sess_t;

#define N3_PDU_INFO_SIZE(a)	(sizeof(n3_pdu_info_t) - sizeof(n3_pdu_sess_t) + a->pdu_num * (sizeof(n3_pdu_sess_t)))
typedef struct n3_pdu_info_t {
#define MAX_N3_PDU_NUM			12
	uint8_t			pdu_num;
	n3_pdu_sess_t	pdu_sessions[MAX_N3_PDU_NUM];
} n3_pdu_info_t;

#pragma pack(pop)
