#ifndef __EAPP_H__
#define __EAPP_H__

#include <n3iwf_comm.h>
#include <eap5g.h>

typedef struct ike_tag_t {
	char        ue_from_addr[INET_ADDRSTRLEN];
	uint16_t    ue_from_port;
	char		up_from_addr[INET_ADDRSTRLEN];
	uint16_t	up_from_port;
	char		cp_amf_host[128];
} ike_tag_t;

typedef union ike_data_t {
	n3_eap_result_t eap_result;
	n3_pdu_info_t	pdu_info;
} ike_data_t;

typedef enum ike_data_choice_t {
	choice_unset		= 0,
	choice_eap_result,
	choice_pdu_info
} ike_data_choice_t;

typedef struct ike_msg_t {
	long				mtype;		/* 1 or worker qid (1,2,3,4...) */
	ike_tag_t			ike_tag;
	n3iwf_msg_t 		n3iwf_msg;	/* warning! not include whole payload */
	ike_data_choice_t	ike_choice;
	ike_data_t			ike_data;
	eap_relay_t 		eap_5g;	
} ike_msg_t;
#define IKE_MSG_SIZE	(sizeof(ike_msg_t) - sizeof(long))

#endif /* __EAPP_H__ */
