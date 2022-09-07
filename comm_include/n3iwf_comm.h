#include <stdio.h>
#include <stdint.h>

#define N3_EXPIRE_TM_SEC		3

typedef enum n3iwf_msgcode_t {
	N3_IKE_AUTH_INIT = 0,
	N3_IKE_AUTH_REQ,
	N3_IKE_AUTH_RES,
	N3_IKE_INFORM_REQ,
	N3_IKE_INFORM_RES,
	N3_CREATE_CHILD_SA_REQ,
	N3_CREATE_CHILD_SA_RES
} n3iwf_msgcode_t;

typedef enum n3iwf_rescode_t {
	N3_EAP_REQUEST = 0,
	N3_EAP_SUCCESS,
	N3_EAP_FAILURE,
	/* .... */
} n3iwf_rescode_t;

typedef struct ctx_info_t {
	uint16_t	up_id;
	uint16_t	cp_id;
} ctx_info_t;

typedef struct n3iwf_msg_t {
	uint8_t		msg_code;
	uint8_t		res_code;
	ctx_info_t	ctx_info;
	uint16_t	payload_len;
	char		payload[1]; 	/* msg_code == N3_IKE_AUTH_INIT ? string(UE_ID\'0') : ... */
} n3iwf_msg_t;

#define N3IWF_MSG_SIZE(a)	(sizeof(n3iwf_msg_t) - 1 + htons(a->payload_len))

/* 각 상세 payload 정의 */
// ...



