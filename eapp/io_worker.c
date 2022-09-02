#include <eapp.h>

extern main_ctx_t *MAIN_CTX;
static __thread worker_ctx_t *WORKER_CTX;

void handle_udp_request(int fd, short event, void *arg)
{   
	recv_buf_t *recv_buf = (recv_buf_t *)arg;

	fprintf(stderr, "{dbg} [%s] recv!\n", WORKER_CTX->thread_name);

	/* for test */
	eap_relay_t eap_5g_nas = {0,};
	decap_eap_res(&eap_5g_nas, recv_buf->buffer, recv_buf->size);

	release_recv_buf(&MAIN_CTX->udp_buff, recv_buf);
}

void io_thrd_tick(evutil_socket_t fd, short events, void *data)
{
    //worker_ctx_t *worker_ctx = (worker_ctx_t *)data;
}

void *io_worker_thread(void *arg)
{
    WORKER_CTX = (worker_ctx_t *)arg;
    WORKER_CTX->evbase_thrd = event_base_new();
                    
    struct timeval one_sec = {2, 0};
    struct event *ev_tick = event_new(WORKER_CTX->evbase_thrd, -1, EV_PERSIST, io_thrd_tick, WORKER_CTX);
    event_add(ev_tick, &one_sec);

    event_base_loop(WORKER_CTX->evbase_thrd, EVLOOP_NO_EXIT_ON_EMPTY);
        
    return NULL;
}
