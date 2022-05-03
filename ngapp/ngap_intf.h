#ifndef __NGAPP_H__
#define __NGAPP_H__

#include <json_macro.h>
#include <sctp_intf.h>

typedef enum ngap_from_t {
	NGAP_FROM_NONE = 0,
	NGAP_FROM_APP,
	NGAP_FROM_SCTP
} ngap_from_t;

typedef struct ngap_tag_t {
	int id;
	key_list_t key_list;
} ngap_tag_t;

#define MAX_NGAP_BUFF_SIZE	65535
typedef struct ngap_msg_t {
	long		mtype;
	ngap_tag_t	ngap_tag;
	sctp_tag_t	sctp_tag;
	size_t		msg_size;
	char		msg_body[MAX_NGAP_BUFF_SIZE];
} ngap_msg_t;
#define NGAP_MSG_SIZE(a)    (sizeof(ngap_msg_t) + sizeof(ngap_tag_t) - MAX_NGAP_BUFF_SIZE + (a)->msg_size - sizeof(long))
#define NGAP_MSG_INFO_SIZE  (sizeof(ngap_tag_t) + sizeof(sctp_tag_t) + sizeof(int) + sizeof(size_t))

#endif /* __NGAPP_H__ */
