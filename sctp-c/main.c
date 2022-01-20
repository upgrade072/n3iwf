#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
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

#include <libutil.h>
#include <libconfig.h>
#include <event.h>
#include <event2/event.h>
#include <event2/thread.h>

typedef struct recv_buf_t {
	int occupied;
	int size;
	char *buffer;
} recv_buf_t;

typedef struct recv_buff_t {
#define MAX_MAIN_BUFF_SIZE	65536
#define MAX_MAIN_BUFF_NUM	65536
	int total_num;
	int used_num;
	int each_size;
	recv_buf_t *buffers;
} recv_buff_t;

typedef struct worker_ctx_t {
	pthread_t pthread_id;
	int  thread_index;
	char thread_name[16];
	struct event_base *evbase_thrd;

	/* for bf_thread */
	recv_buff_t recv_buff;
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
	ppid_pqid_t *recv_relay;
} qid_info_t;

typedef struct main_ctx_t {
	config_t CFG;
	qid_info_t QID_INFO;
	worker_thread_t BF_WORKERS;
	worker_thread_t IO_WORKERS;
	struct event_base *evbase_main;
} main_ctx_t;
	
main_ctx_t main_ctx, *MAIN_CTX = &main_ctx;

void *io_worker_thread(void *arg)
{
	while (1) {
		sleep(1);
	}
}

void *bf_worker_thread(void *arg)
{
	while (1) {
		sleep(1);
	}
}

int create_worker(worker_thread_t *WORKER, const char *prefix, int worker_num)
{
	int io_worker = 0, bf_worker = 0;
	if (!strcmp(prefix, "io_worker")) {
		io_worker = 1;
	} else if (!strcmp(prefix, "bf_worker")) {
		bf_worker = 1;
	}
	if (!io_worker && !bf_worker) {
		fprintf(stderr, "%s() fatal! recv unhandle worker request=[%s]\n", __func__, prefix);
		return (-1);
	}
	if (worker_num <= 0 || worker_num > MAX_WORKER_NUM) {
		fprintf(stderr, "%s() fatal! [%s] worker_num=(%d/max=%d)!\n", __func__, prefix, worker_num, MAX_WORKER_NUM);
		return (-1);
	}

	WORKER->workers = malloc(sizeof(worker_ctx_t) * worker_num);

	for (int i = 0; i < worker_num; i++) {
		worker_ctx_t *worker_ctx = &WORKER->workers[i];
		worker_ctx->thread_index = i;
		sprintf(worker_ctx->thread_name, "%s_%02d", prefix, i);
		if (pthread_create(&worker_ctx->pthread_id, NULL, 
					io_worker ? io_worker_thread : bf_worker_thread,
					worker_ctx)) {
			fprintf(stderr, "%s() fatal! fail to create thread=(%s)\n", __func__, worker_ctx->thread_name);
		} else {
			pthread_setname_np(worker_ctx->pthread_id, worker_ctx->thread_name);
			fprintf(stderr, "setname=[%s]\n", worker_ctx->thread_name);
		}
	}

	return 0;
}

int initialize(main_ctx_t *MAIN_CTX)
{
	/* loading config */
	config_init(&MAIN_CTX->CFG);
	if (!config_read_file(&MAIN_CTX->CFG, "./sctp_c.cfg")) {
		fprintf(stderr, "%s() fatal! fail to load cfg file=(%s) line:text(%d/%s)!\n",
			__func__,
			config_error_file(&MAIN_CTX->CFG),
			config_error_line(&MAIN_CTX->CFG),
			config_error_text(&MAIN_CTX->CFG));

		return (-1);
	} else {
		fprintf(stderr, "%s() load cfg ---------------------\n", __func__);
		config_write(&MAIN_CTX->CFG, stderr);
		fprintf(stderr, "===========================================\n");

		config_set_options(&MAIN_CTX->CFG, 0);
		config_set_tab_width(&MAIN_CTX->CFG, 4);
		config_write_file(&MAIN_CTX->CFG, "./sctp_c.cfg"); // save cfg with indent
	}

	/* check sctp support */
	int sctp_test = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (sctp_test < 0) {
		fprintf(stderr, "%s() fatal! sctp not supported! run <checksctp>, run <modprobe sctp>\n", __func__);
	} else {
		close(sctp_test);
	}

#if 0
	/* create main recv queue */
	config_lookup_int(&MAIN_CTX->CFG, "process_config.queue_size", &MAIN_CTX->MAIN_BUFF.each_size);
	config_lookup_int(&MAIN_CTX->CFG, "process_config.queue_count", &MAIN_CTX->MAIN_BUFF.total_num);
	if ((MAIN_CTX->MAIN_BUFF.each_size <= 0 || MAIN_CTX->MAIN_BUFF.each_size > MAX_MAIN_BUFF_SIZE) ||
		(MAIN_CTX->MAIN_BUFF.total_num <= 0 || MAIN_CTX->MAIN_BUFF.total_num > MAX_MAIN_BUFF_NUM)) {
		fprintf(stderr, "%s() fatal! process_config.queue_size=(%d/max=%d) process_config.queue_count=(%d/max=%d)!\n",
				__func__, 
				MAIN_CTX->MAIN_BUFF.each_size, MAX_MAIN_BUFF_SIZE,
				MAIN_CTX->MAIN_BUFF.total_num, MAX_MAIN_BUFF_NUM);
		return (-1);
	} else {
		MAIN_CTX->MAIN_BUFF.buffers = malloc(sizeof(recv_buf_t) * MAIN_CTX->MAIN_BUFF.total_num);
		for (int i = 0; i < MAIN_CTX->MAIN_BUFF.total_num; i++) {
			recv_buf_t *buffer = &MAIN_CTX->MAIN_BUFF.buffers[i];
			buffer->occupied = 0;
			buffer->size = 0;
			buffer->buffer = malloc(MAIN_CTX->MAIN_BUFF.each_size);
		}
	}
#endif

	/* create queue_id_info */
	int queue_key = 0;
	config_lookup_int(&MAIN_CTX->CFG, "queue_id_info.send_relay", &queue_key);
	if (queue_key < 0 || (MAIN_CTX->QID_INFO.send_relay = msgget(queue_key, IPC_CREAT|0666)) < 0) {
		fprintf(stderr, "%s() fatal! queue_id_info.send_relay, cant create/attach send_relay queue key=(0x%x)\n", __func__, queue_key);
		return (-1);
	} else {
		util_print_msgq_info(queue_key, MAIN_CTX->QID_INFO.send_relay);
	}
	config_setting_t *conf_recv_relay = config_lookup(&MAIN_CTX->CFG, "queue_id_info.recv_relay");
	int recv_relay_num = conf_recv_relay == NULL ? 0 : config_setting_length(conf_recv_relay);
	if (recv_relay_num <= 0) {
		fprintf(stderr, "%s() fatal! queue_id_info.recv_relay not exist!\n", __func__);
		return (-1);
	} else {

		MAIN_CTX->QID_INFO.recv_relay = malloc(sizeof(qid_info_t) * recv_relay_num);
		// TODO
	}

	/* create io workers */
	config_lookup_int(&MAIN_CTX->CFG, "process_config.io_worker_num", &MAIN_CTX->IO_WORKERS.worker_num);
	if (create_worker(&MAIN_CTX->IO_WORKERS, "io_worker", MAIN_CTX->IO_WORKERS.worker_num) < 0) {
		return (-1);
	}

	/* create bf worker */
	config_lookup_int(&MAIN_CTX->CFG, "process_config.bf_worker_num", &MAIN_CTX->BF_WORKERS.worker_num);
	if (create_worker(&MAIN_CTX->BF_WORKERS, "bf_worker", MAIN_CTX->BF_WORKERS.worker_num) < 0) {
		return (-1);
	}

	return (0);
}

int main()
{
	evthread_use_pthreads();

	if (initialize(MAIN_CTX) < 0) {
		exit(0);
	}

	/* create main */
	MAIN_CTX->evbase_main = event_base_new();

	while (1) {
		sleep (1);
	}
}
