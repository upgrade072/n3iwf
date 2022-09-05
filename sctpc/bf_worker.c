#include "sctpc.h"

extern main_ctx_t *MAIN_CTX;
static __thread worker_ctx_t *WORKER_CTX;

recv_buf_t *find_empty_recv_buffer(worker_ctx_t *worker_ctx)
{
	for (int i = 0; i < worker_ctx->recv_buff.total_num; i++) {
		recv_buf_t *buffer = &worker_ctx->recv_buff.buffers[ (worker_ctx->recv_buff.curr_index + i) % worker_ctx->recv_buff.total_num ];
		if (buffer->occupied == 0)  {
			worker_ctx->recv_buff.curr_index = (i + 1) % worker_ctx->recv_buff.total_num;
			return buffer;
		};
	}

	return NULL;
}

int relay_to_io_worker(recv_buf_t *buffer, main_ctx_t *MAIN_CTX)
{
	conn_curr_t *CURR_CONN = take_conn_list(MAIN_CTX);
	sctp_msg_t *sctp_msg = (sctp_msg_t *)buffer->buffer;

	conn_info_t find_info = {0,};
	sprintf(find_info.name, "%s", sctp_msg->tag.hostname);

	conn_info_t *conn_info = bsearch(&find_info, CURR_CONN->dist_info, CURR_CONN->rows_num, sizeof(conn_info_t), sort_conn_list);
	if (conn_info != NULL) {
		worker_ctx_t *worker_ctx = &MAIN_CTX->IO_WORKERS.workers[conn_info->id % MAIN_CTX->IO_WORKERS.worker_num];
		return event_base_once(worker_ctx->evbase_thrd, -1, EV_TIMEOUT, handle_sock_send, buffer, NULL);
	}
	return (-1);
}

void bf_msgq_read(int fd, short events, void *data)
{
	worker_ctx_t *worker_ctx = (worker_ctx_t *)data;

	/* check if there exist something to send */
	struct msqid_ds msgq_stat = {{0,}};
	while ((msgctl(MAIN_CTX->QID_INFO.sctp_send_relay, IPC_STAT, &msgq_stat) >= 0) 
			&& (msgq_stat.msg_qnum > 0)) {

		/* find empty slot */
		recv_buf_t *buffer = find_empty_recv_buffer(worker_ctx);
		if (buffer == NULL) return;

		/* receive race */
		buffer->size = msgrcv(MAIN_CTX->QID_INFO.sctp_send_relay, buffer->buffer, worker_ctx->recv_buff.each_size, 0, IPC_NOWAIT);

		/* find dest-conn status */
			/* if unabe reply to sender or discard */
		if (buffer->size <= 0) continue;
		if (buffer->size < (SCTP_MSG_INFO_SIZE + 1)) continue;

		/* if set occupied = 1 to slot & relay to io_worker */
		buffer->occupied = 1;
		if (relay_to_io_worker(buffer, MAIN_CTX) < 0) {
			buffer->occupied = 0;
		}
	}
}

void bf_worker_init(worker_ctx_t *worker_ctx, main_ctx_t *MAIN_CTX)
{
	struct timeval one_u_sec = {0, 1};
	struct event *ev_msgq_read = event_new(WORKER_CTX->evbase_thrd, -1, EV_PERSIST, bf_msgq_read, worker_ctx);
	event_add(ev_msgq_read, &one_u_sec);
}

void bf_thrd_tick(evutil_socket_t fd, short events, void *data)
{
    //worker_ctx_t *worker_ctx = (worker_ctx_t *)data;
}

void *bf_worker_thread(void *arg)
{
	WORKER_CTX = (worker_ctx_t *)arg;
	WORKER_CTX->evbase_thrd = event_base_new();

	bf_worker_init(WORKER_CTX, MAIN_CTX);

	struct timeval one_sec = {2, 0};
	struct event *ev_tick = event_new(WORKER_CTX->evbase_thrd, -1, EV_PERSIST, bf_thrd_tick, WORKER_CTX);
	event_add(ev_tick, &one_sec);

	event_base_loop(WORKER_CTX->evbase_thrd, EVLOOP_NO_EXIT_ON_EMPTY);

	return NULL;
}

