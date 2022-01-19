#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <errno.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/queue.h>

#include <libutil.h>
#include <libconfig.h>

typedef struct recv_buf_t {
	int occupied;
	int size;
	char *buffer;
} recv_buf_t;

typedef struct main_buff_t {
#define MAX_MAIN_BUFF_SIZE	65536
#define MAX_MAIN_BUFF_NUM	65536
	int total_num;
	int used_num;
	int each_size;
	recv_buf_t *buffers;
} main_buff_t;

typedef struct worker_ctx_t {
	pthread_t pthread_id;
	int  thread_index;
	char thread_name[16];
} worker_ctx_t;

typedef struct worker_thread_t {
#define MAX_WORKER_NUM		12
#define WORKER_PREFIX		"sctp_c_worker_"
	int worker_num;
	worker_ctx_t *workers;
} worker_thread_t;

typedef struct main_ctx_t {
	config_t CFG;
	main_buff_t MAIN_BUFF;
	worker_thread_t WORKERS;
} main_ctx_t;
	
main_ctx_t main_ctx, *MAIN_CTX = &main_ctx;

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

		goto INITIALIZE_ERR;
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

	/* create main recv queue */
	config_lookup_int(&MAIN_CTX->CFG, "process_config.queue_size", &MAIN_CTX->MAIN_BUFF.each_size);
	config_lookup_int(&MAIN_CTX->CFG, "process_config.queue_count", &MAIN_CTX->MAIN_BUFF.total_num);
	if ((MAIN_CTX->MAIN_BUFF.each_size <= 0 || MAIN_CTX->MAIN_BUFF.each_size > MAX_MAIN_BUFF_SIZE) ||
		(MAIN_CTX->MAIN_BUFF.total_num <= 0 || MAIN_CTX->MAIN_BUFF.total_num > MAX_MAIN_BUFF_NUM)) {
		fprintf(stderr, "%s() fatal! process_config.queue_size=(%d/max=%d) process_config.queue_count=(%d/max=%d)!\n",
				__func__, 
				MAIN_CTX->MAIN_BUFF.each_size, MAX_MAIN_BUFF_SIZE,
				MAIN_CTX->MAIN_BUFF.total_num, MAX_MAIN_BUFF_NUM);
		goto INITIALIZE_ERR;
	} else {
		MAIN_CTX->MAIN_BUFF.buffers = malloc(sizeof(recv_buf_t) * MAIN_CTX->MAIN_BUFF.total_num);
		for (int i = 0; i < MAIN_CTX->MAIN_BUFF.total_num; i++) {
			recv_buf_t *buffer = &MAIN_CTX->MAIN_BUFF.buffers[i];
			buffer->occupied = 0;
			buffer->size = 0;
			buffer->buffer = malloc(MAIN_CTX->MAIN_BUFF.each_size);
		}
	}

	/* create workers */

	return (0);

INITIALIZE_ERR:
	config_destroy(&MAIN_CTX->CFG);
	return (-1);
}

int main()
{
	initialize(MAIN_CTX);
}
