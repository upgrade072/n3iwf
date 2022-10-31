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
#include <libnode.h>
#include <libutil.h>

#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json_macro.h>

#include <event.h>
#include <event2/event.h>
#include <event2/thread.h>

#include <sctp_intf.h>
#include "ngap_intf.h"
#include <libngapp.h>

/* for NGAP en/decode */
#include <NGAP-PDU-Descriptions.h>

#include <commlib.h>
#include <mmclib.h>
#include <overloadlib.h>
#include <msgtrclib.h>
#include <stm_msgtypes.h>

/* for STAT count */
typedef enum ngap_pdu_type_t {
	NPTE_ngapPduTypeUnused = 0,
	NPTE_initiatingMessage = 1,
	NPTE_successfulOutcome,
	NPTE_unsuccessfulOutcome,
	NPTE_ngapPduTypeUnknown
} ngap_pdu_type_t;

typedef enum ngap_stat_type_t {
	NGAP_STAT_RECV		= 0,
	NGAP_STAT_SEND,
	NGAP_STAT_NUM
} ngap_stat_type_t;

typedef struct ngap_op_count_t {
	int count[NPTE_ngapPduTypeUnknown][NGAP_ProcCodeUnknown];
} ngap_op_count_t;

typedef struct ngap_stat_t {
#define NGAP_STAT_SLOT	2		/* 0 1 0 1 ... */
	int				ngap_stat_pos; 
	ngap_op_count_t stat[NGAP_STAT_SLOT][NGAP_STAT_NUM];
} ngap_stat_t;
/* for STAT count */


#define MAX_WORKER_NUM      12

#define MAX_DISTR_BUFF_SIZE 65536
#define MAX_DISTR_BUFF_NUM  65536

typedef struct recv_buf_t {
    int occupied;
    int size;
    char *buffer;
} recv_buf_t;

typedef struct recv_buff_t {
    int total_num;
    int used_num;
    int each_size;
    int curr_index;
    recv_buf_t *buffers;
} recv_buff_t;

typedef struct qid_info_t {
	int main_qid;
	int ixpc_qid;
	int ngapp_sctpc_qid;
	int sctpc_ngapp_qid;
	int ngapp_nwucp_qid;
	int nwucp_ngapp_qid;
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

	recv_buff_t recv_buff;

	ngap_stat_t ngap_stat;
} worker_ctx_t;

typedef struct worker_thread_t {
	int worker_num;
	worker_ctx_t *workers;
} worker_thread_t;

typedef struct main_ctx_t {
	config_t CFG;
	qid_info_t QID_INFO;
	nwucp_distr_t DISTR_INFO;
	worker_thread_t BF_WORKERS;
	worker_thread_t IO_WORKERS;
	struct event_base *evbase_main;

	/* for ngapp asn1 */
	struct ossGlobal world;
	int pdu_num;
} main_ctx_t;

/* ------------------------- stat.c --------------------------- */
int     ngap_pdu_proc_code(NGAP_PDU *ngap_pdu);
void    NGAP_STAT_COUNT(NGAP_PDU *ngap_pdu, ngap_stat_type_t stat_type);
void    NGAP_STAT_GATHER(main_ctx_t *MAIN_CTX, ngap_op_count_t STAT[NGAP_STAT_NUM]);
void    NGAP_STAT_PRINT(ngap_op_count_t STAT[NGAP_STAT_NUM]);
void    send_conn_stat(main_ctx_t *MAIN_CTX, IxpcQMsgType *rxIxpcMsg);

/* ------------------------- bf_worker.c --------------------------- */
int     select_io_worker_index();
recv_buf_t      *find_empty_recv_buffer(worker_ctx_t *worker_ctx);
int     relay_to_io_worker(recv_buf_t *buffer, main_ctx_t *MAIN_CTX, int from);
void    bf_msgq_read(int fd, short events, void *data);
void    bf_worker_init(worker_ctx_t *worker_ctx, main_ctx_t *MAIN_CTX);
void    bf_thrd_tick(evutil_socket_t fd, short events, void *data);
void    *bf_worker_thread(void *arg);

/* ------------------------- io_worker.c --------------------------- */
void    handle_ngap_send(int conn_fd, short events, void *data);
json_object     *extract_distr_key(json_object *js_ngap_pdu, key_list_t *key_list);
void    handle_ngap_recv(int conn_fd, short events, void *data);
void    io_thrd_tick(evutil_socket_t fd, short events, void *data);
void    *io_worker_thread(void *arg);

/* ------------------------- main.c --------------------------- */
int     create_worker_recv_queue(worker_ctx_t *worker_ctx, main_ctx_t *MAIN_CTX);
int     create_worker_thread(worker_thread_t *WORKER, const char *prefix, main_ctx_t *MAIN_CTX);
int     initialize(main_ctx_t *MAIN_CTX);
void    main_tick(int conn_fd, short events, void *data);
void    main_msgq_read_callback(evutil_socket_t fd, short events, void *data);
int     main();
