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

// for NGAP en/decode
#include <NGAP-PDU-Descriptions.h>

#define MAX_WORKER_NUM      12

typedef struct recv_buf_t {
    int occupied;
    int size;
    char *buffer;
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
	int ngapp_sctpc_qid;
	int sctpc_ngapp_qid;
	int ngapp_nwucp_qid;
	int nwucp_ngapp_qid;
} qid_info_t;

typedef struct ngap_distr_t {
	const char *worker_rule;
	int worker_num;
	int worker_distr_qid[MAX_WORKER_NUM];
} ngap_distr_t;

typedef struct worker_ctx_t {
	pthread_t pthread_id;
	int thread_index;
	char thread_name[16];
	struct event_base *evbase_thrd;

	recv_buff_t recv_buff;
} worker_ctx_t;

typedef struct worker_thread_t {
	int worker_num;
	worker_ctx_t *workers;
} worker_thread_t;

typedef struct main_ctx_t {
	config_t CFG;
	qid_info_t QID_INFO;
	ngap_distr_t DISTR_INFO;
	worker_thread_t BF_WORKERS;
	worker_thread_t IO_WORKERS;
	struct event_base *evbase_main;

	/* for ngapp asn1 */
	struct ossGlobal world;
	int pdu_num;
} main_ctx_t;

#define MAX_DISTR_BUFF_SIZE 65536
#define MAX_DISTR_BUFF_NUM  65536

/* ------------------------- main.c --------------------------- */
int     create_worker_recv_queue(worker_ctx_t *worker_ctx, main_ctx_t *MAIN_CTX);
int     create_worker_thread(worker_thread_t *WORKER, const char *prefix, main_ctx_t *MAIN_CTX);
int     initialize(main_ctx_t *MAIN_CTX);
int     main();

/* ------------------------- io_worker.c --------------------------- */
void    handle_ngap_send(int conn_fd, short events, void *data);
void    handle_ngap_recv(int conn_fd, short events, void *data);
void    io_thrd_tick(evutil_socket_t fd, short events, void *data);
void    *io_worker_thread(void *arg);

/* ------------------------- bf_worker.c --------------------------- */
int     select_io_worker_index();
recv_buf_t      *find_empty_recv_buffer(worker_ctx_t *worker_ctx);
int     relay_to_io_worker(recv_buf_t *buffer, main_ctx_t *MAIN_CTX, int from);
void    bf_msgq_read(int fd, short events, void *data);
void    bf_worker_init(worker_ctx_t *worker_ctx, main_ctx_t *MAIN_CTX);
void    bf_thrd_tick(evutil_socket_t fd, short events, void *data);
void    *bf_worker_thread(void *arg);
