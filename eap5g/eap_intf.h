#ifndef __EAPP_H__
#define __EAPP_H__

#include <n3iwf_comm.h>
#include <eap5g.h>

typedef struct ike_tag_t {
	char		ue_id[128];
	char		from_addr[INET_ADDRSTRLEN];
	int			from_port;
	char		amf_host[128];
} ike_tag_t;

typedef struct ike_msg_t {
	long		mtype;		/* 1 or worker qid (1,2,3,4...) */
	ike_tag_t	ike_tag;
	n3iwf_msg_t n3iwf_msg;	/* warning! not include whole payload */
	eap_relay_t eap_5g;	
} ike_msg_t;
#define IKE_MSG_SIZE	(sizeof(ike_msg_t) - sizeof(long))

#endif /* __EAPP_H__ */
