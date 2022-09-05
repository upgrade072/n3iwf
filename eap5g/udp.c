#include <eapp.h>

extern main_ctx_t *MAIN_CTX;

static int IO_WORKER_DISTR;

void udp_sock_read_callback(int fd, short event, void *arg)
{ 
	(void)event;
	(void)arg;

	recv_buf_t *recv_buf = NULL;

	socklen_t fl = sizeof(struct sockaddr_in);

	if ((recv_buf = get_recv_buf(&MAIN_CTX->udp_buff)) == NULL) {
		fprintf(stderr, "{dbg} oops~ %s() can't get recv_buf!\n", __func__);
		goto USRC_ERR;
	}

	if ((recv_buf->size = 
			recvfromto(fd, recv_buf->buffer, MAIN_CTX->udp_buff.each_size, 0, 
			(struct sockaddr *)&recv_buf->from_addr, &fl, NULL, NULL)) < 0) {
		fprintf(stderr, "{dbg} oops~ %s() err (%d:%s)\n", __func__, errno, strerror(errno));
		goto USRC_ERR;
	}

	/* relay to io_worker */
	IO_WORKER_DISTR = (IO_WORKER_DISTR + 1) % MAIN_CTX->IO_WORKERS.worker_num;
	worker_ctx_t *io_worker = &MAIN_CTX->IO_WORKERS.workers[IO_WORKER_DISTR];
	event_base_once(io_worker->evbase_thrd, -1, EV_TIMEOUT, handle_udp_request, recv_buf, NULL);
	return;

USRC_ERR:
	if (recv_buf != NULL) {
		release_recv_buf(&MAIN_CTX->udp_buff, recv_buf);
	}
}
