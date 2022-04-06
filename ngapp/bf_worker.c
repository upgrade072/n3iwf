#include "ngapp.h"

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
	return (-1);
}

void bf_msgq_read(int fd, short events, void *data)
{
    worker_ctx_t *worker_ctx = (worker_ctx_t *)data;
	int QID = worker_ctx->thread_index == 0 ? MAIN_CTX->QID_INFO.ngap_recv_qid : MAIN_CTX->QID_INFO.sctp_recv_qid;

    /* check if there exist something to send */
    struct msqid_ds msgq_stat = {{0,}};
    while ((msgctl(QID, IPC_STAT, &msgq_stat) >= 0)
            && (msgq_stat.msg_qnum > 0)) {

        /* find empty slot */
        recv_buf_t *buffer = find_empty_recv_buffer(worker_ctx);
        if (buffer == NULL) return;

        buffer->size = msgrcv(QID, buffer->buffer, worker_ctx->recv_buff.each_size, 0, IPC_NOWAIT);

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

