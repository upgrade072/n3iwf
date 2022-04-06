#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <errno.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/queue.h>

#include <libnode.h>
#include <libutil.h>

#include <libconfig.h>
#include <fort.h>
#include <event.h>
#include <event2/event.h>
#include <event2/thread.h>

#include "sctp_intf.h"

typedef union {
	struct sockaddr_storage ss;
	struct sockaddr_in v4;
	struct sockaddr_in6 v6;
	struct sockaddr sa;
} sockaddr_storage_t;

typedef struct recv_buf_t {
	int occupied;
	int size;
	char *buffer;
} recv_buf_t;

typedef struct recv_buff_t {
#define MAX_SCTP_BUFF_NUM	65536
	int total_num;
	int used_num;
	int each_size;
	int curr_index;
	recv_buf_t *buffers;
} recv_buff_t;

typedef struct worker_ctx_t {
	pthread_t pthread_id;
	int  thread_index;
	char thread_name[16];
	struct event_base *evbase_thrd;

	linked_list conn_list; /* for io thread */
	recv_buff_t recv_buff; /* for bf thread */
} worker_ctx_t;

typedef struct worker_thread_t {
#define MAX_WORKER_NUM		12
	int worker_num;
	worker_ctx_t *workers;
} worker_thread_t;

typedef struct ppid_pqid_t {
	int sctp_ppid;
	int proc_pqid;
} ppid_pqid_t;

typedef struct qid_info_t {
	int send_relay;
	int recv_relay_num;
	ppid_pqid_t *recv_relay;
} qid_info_t;

typedef struct tailq_entry {
	TAILQ_ENTRY(tailq_entry) items;
	recv_buf_t *send_item;
} tailq_entry;

typedef struct conn_status_t {
	int conns_fd;
	int assoc_id;
	int assoc_state;
	void *sctp_client;
	struct event *conn_ev;
	TAILQ_HEAD(, tailq_entry) queue_head;
} conn_status_t;

typedef struct conn_info_t {
	int  occupied;
	int  id;				/* distribute handle by thread_index */
	char name[128];
	int  enable;
	int  conn_num;
#define MAX_SC_ADDR_NUM		3
	char src_addr[MAX_SC_ADDR_NUM][128];
	char dst_addr[MAX_SC_ADDR_NUM][128];
	int  sport;
	int  dport;
#define MAX_SC_CONN_NUM		12
	conn_status_t conn_status[MAX_SC_CONN_NUM];
} conn_info_t;

typedef struct conn_curr_t {
	int rows_num;
#define MAX_SC_CONN_LIST	128
	conn_info_t conn_info[MAX_SC_CONN_LIST]; /* conn status ~ io_worker */
	conn_info_t dist_info[MAX_SC_CONN_LIST]; /* sorted list ~ bf_worker */
} conn_curr_t;

typedef struct conn_list_t {
	int curr_pos;
#define MAX_SC_CONN_SLOT	2
	conn_curr_t curr_conn[MAX_SC_CONN_SLOT];
} conn_list_t;

typedef struct sctp_client_t {
	int id;
	char name[128];
	int enable;

	int src_addr_num;
	int dst_addr_num;
	struct sockaddr_in src_addr[MAX_SC_ADDR_NUM];
	struct sockaddr_in dst_addr[MAX_SC_ADDR_NUM];

	int conn_num;
	int curr_index;
	conn_status_t conn_status[MAX_SC_CONN_NUM];
} sctp_client_t;

typedef struct main_ctx_t {
	config_t CFG;
	qid_info_t QID_INFO;
	worker_thread_t BF_WORKERS;
	worker_thread_t IO_WORKERS;
	conn_list_t *SHM_SCTPC_CONN; // for application ref
	struct event_base *evbase_main;
} main_ctx_t;


/* ------------------------- io_worker.c --------------------------- */
void    handle_sock_send(int conn_fd, short events, void *data);
void    handle_sock_recv(int conn_fd, short events, void *data);
void    clear_connection(conn_status_t *conn_status);
conn_status_t   *search_connection(sctp_client_t *client, sctp_msg_t *sctp_msg);
void    check_connection(worker_ctx_t *worker_ctx);
void    client_new(conn_status_t *conn_status, worker_ctx_t *worker_ctx, sctp_client_t *client);
void    create_client(worker_ctx_t *worker_ctx, conn_info_t *conn);
void    io_worker_init(worker_ctx_t *worker_ctx, main_ctx_t *MAIN_CTX);
void    io_thrd_tick(evutil_socket_t fd, short events, void *data);
void    *io_worker_thread(void *arg);

/* ------------------------- list.c --------------------------- */
conn_curr_t     *take_conn_list(main_ctx_t *MAIN_CTX);
int     sort_conn_list(const void *a, const void *b);
void    disp_conn_list(main_ctx_t *MAIN_CTX);
void    init_conn_list(main_ctx_t *MAIN_CTX);

/* ------------------------- stack.c --------------------------- */
int     sctp_noti_assoc_change(struct sctp_assoc_change *sac, const char **event_state);
int     sctp_noti_peer_addr_change(struct sctp_paddr_change *spc, const char **event_state);
int     handle_sctp_notification(union sctp_notification *notif, size_t notif_len, const char **event_str, const char **event_state);
char 	*get_path_state_str(int state);
int     get_assoc_state(int sd, int assoc_id, const char **state_str);

/* ------------------------- bf_worker.c --------------------------- */
recv_buf_t      *find_empty_recv_buffer(worker_ctx_t *worker_ctx);
int     relay_to_io_worker(recv_buf_t *buffer, main_ctx_t *MAIN_CTX);
void    bf_msgq_read(int fd, short events, void *data);
void    bf_worker_init(worker_ctx_t *worker_ctx, main_ctx_t *MAIN_CTX);
void    bf_thrd_tick(evutil_socket_t fd, short events, void *data);
void    *bf_worker_thread(void *arg);

/* ------------------------- main.c --------------------------- */
int     create_worker_recv_queue(worker_ctx_t *worker_ctx, main_ctx_t *MAIN_CTX);
int     create_worker_thread(worker_thread_t *WORKER, const char *prefix, main_ctx_t *MAIN_CTX);
int     initialize(main_ctx_t *MAIN_CTX);
