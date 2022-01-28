typedef struct sctp_tag_t {
	char hostname[64];
	int  assoc_id;
	int	 stream_id;
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
#define SCTP_MSG_SIZE(a)	(sizeof(sctp_msg_t) - MAX_SCTP_BUFF_SIZE + (a)->msg_size - sizeof(long))
#define SCTP_MSG_INFO_SIZE	(sizeof(sctp_tag_t) + sizeof(int) + sizeof(size_t))
