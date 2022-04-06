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
#include <event.h>
#include <event2/event.h>
#include <event2/thread.h>

#include <sctp_intf.h>

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
	int ngap_recv_qid;
	int ngap_send_qid;
	int sctp_recv_qid;
	int sctp_send_qid;
} qid_info_t;

typedef struct worker_ctx_t {
	pthread_t pthread_id;
	int thread_index;
	char thread_name[16];
	struct event_base *evbase_thrd;

	recv_buff_t recv_buff;
} worker_ctx_t;

typedef struct worker_thread_t {
#define MAX_WORKER_NUM      12
	int worker_num;
	worker_ctx_t *workers;
} worker_thread_t;

typedef struct main_ctx_t {
	config_t CFG;
	qid_info_t QID_INFO;
	worker_thread_t BF_WORKERS;
	worker_thread_t IO_WORKERS;
	struct event_base *evbase_main;
} main_ctx_t;

#define MAX_DISTR_BUFF_SIZE 65536
#define MAX_DISTR_BUFF_NUM  65536

/* ------------------------- bf_worker.c --------------------------- */
void    *bf_worker_thread(void *arg);

/* ------------------------- io_worker.c --------------------------- */
void    *io_worker_thread(void *arg);

/* ------------------------- main.c --------------------------- */
int     create_worker_recv_queue(worker_ctx_t *worker_ctx, main_ctx_t *MAIN_CTX);
int     create_worker_thread(worker_thread_t *WORKER, const char *prefix, main_ctx_t *MAIN_CTX);
int     initialize(main_ctx_t *MAIN_CTX);
int     main();
