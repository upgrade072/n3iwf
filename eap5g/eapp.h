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

#include <libconfig.h>
#include <libutil.h>
#include <libudp.h>

#include <event.h>
#include <event2/event.h>
#include <event2/thread.h>

#include <eap_intf.h>

#include <commlib.h>
#include <mmclib.h>
#include <overloadlib.h>
#include <msgtrclib.h>
#include <stm_msgtypes.h>

#define MAX_WORKER_NUM      12

#define MAX_DISTR_BUFF_SIZE 65536
#define MAX_DISTR_BUFF_NUM  65536

typedef struct recv_buf_t {
    int occupied;
    int size;
	struct sockaddr_in from_addr;
    unsigned char *buffer;
} recv_buf_t;

typedef struct recv_buff_t {
#define MAX_DISTR_BUFF_NUM   65536
    int total_num;
    int used_num;
    int each_size;
    int curr_index;
    recv_buf_t *buffers;
} recv_buff_t;

typedef struct qid_info_t {
    int eap5g_nwucp_qid;
    int nwucp_eap5g_qid;
} qid_info_t;

typedef struct nwucp_distr_t {
    int worker_num;
    int worker_distr_qid;
} nwucp_distr_t;

typedef struct worker_ctx_t {
    pthread_t pthread_id;
    int thread_index;
    char thread_name[16];
    struct event_base *evbase_thrd;

	int udp_sock;
    recv_buff_t recv_buff;
} worker_ctx_t;

typedef struct worker_thread_t {
    int worker_num;
    worker_ctx_t *workers;
} worker_thread_t;

typedef struct main_ctx_t {
	config_t CFG;
    qid_info_t QID_INFO;
    nwucp_distr_t DISTR_INFO;
	worker_thread_t IO_WORKERS;

	struct event_base *evbase_main;
	int udp_listen_port;
	int udp_sock;

	recv_buff_t udp_buff;

	int ike_listen_port;
} main_ctx_t;


/* ------------------------- comm.c --------------------------- */
recv_buf_t      *get_recv_buf(recv_buff_t *recv_buff);
void    release_recv_buf(recv_buff_t *recv_buff, recv_buf_t *recv_buf);

/* ------------------------- io_worker.c --------------------------- */
void    handle_udp_request(int fd, short event, void *arg);
void    handle_ike_request(ike_msg_t *ike_msg);
void    msg_rcv_from_nwucp(int fd, short event, void *arg);
void    io_thrd_tick(evutil_socket_t fd, short events, void *data);
void    *io_worker_thread(void *arg);

/* ------------------------- intf.c --------------------------- */
void    udp_sock_read_callback(int fd, short event, void *arg);
void    handle_pkt_ntohs(n3iwf_msg_t *n3iwf_msg);
void    handle_pkg_htons(n3iwf_msg_t *n3iwf_msg);
void    create_ike_tag(ike_tag_t *ike_tag, struct sockaddr_in *from_addr);
const   char *n3_msg_code_str(int msg_code);
const   char *n3_res_code_str(int res_code);
const   char *eap5g_msg_id_str(int msg_id);

/* ------------------------- trace.c --------------------------- */
size_t  trace_ike_msg(char *body, ike_msg_t *ike_msg);

/* ------------------------- main.c --------------------------- */
int     create_worker_thread(worker_thread_t *WORKER, const char *prefix, main_ctx_t *MAIN_CTX);
int     initialize(main_ctx_t *MAIN_CTX);
int     main();

/* ------------------------- proc.c --------------------------- */
void    proc_eap_init(n3iwf_msg_t *n3iwf_msg, ike_msg_t *ike_msg);
void    proc_eap_request(ike_msg_t *ike_msg, n3iwf_msg_t *n3iwf_msg);
void    proc_eap_response(n3iwf_msg_t *n3iwf_msg, ike_msg_t *ike_msg);
void    proc_ipsec_noti(n3iwf_msg_t *n3iwf_msg, ike_msg_t *ike_msg);
void    proc_inform_response(n3iwf_msg_t *n3iwf_msg, ike_msg_t *ike_msg);
void    proc_eap_result(ike_msg_t *ike_msg, n3iwf_msg_t *n3iwf_msg);
void    proc_inform_request(ike_msg_t *ike_msg, n3iwf_msg_t *n3iwf_msg);
void    proc_pdu_request(ike_msg_t *ike_msg, n3iwf_msg_t *n3iwf_msg);
void    proc_pdu_response(n3iwf_msg_t *n3iwf_msg, ike_msg_t *ike_msg);
void    proc_udp_request(int msg_code, int res_code, n3iwf_msg_t *n3iwf_msg, ike_msg_t *ike_msg);
void    proc_msg_request(int msg_code, int res_code, ike_msg_t *ike_msg, n3iwf_msg_t *n3iwf_msg);
