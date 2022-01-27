typedef struct sctp_tag_t {
	char hostname[128];
	int  assoc_id;
	int  ppid;
} sctp_tag_t;

#define MAX_SCTP_BUFF_SIZE	65536
typedef struct sctp_msg_t {
	long		mtype;
	sctp_tag_t tag;
	int			msg_type;
	size_t		msg_size;
	char		msg_body[MAX_SCTP_BUFF_SIZE];
} sctp_msg_t;
