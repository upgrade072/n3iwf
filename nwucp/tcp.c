#include <nwucp.h>

extern main_ctx_t *MAIN_CTX;

extern __thread worker_ctx_t *WORKER_CTX;

sock_ctx_t *create_sock_ctx(int fd, struct sockaddr *sa, int ue_index)
{
	sock_ctx_t *sock_ctx = calloc(1, sizeof(sock_ctx_t));
	sock_ctx->ue_index = ue_index;
	sock_ctx->sock_fd = fd;
	sock_ctx->connected = 0;
	sprintf(sock_ctx->client_ipaddr, "%s", util_get_ip_from_sa(sa));
	sock_ctx->client_port = util_get_port_from_sa(sa);
	time(&sock_ctx->create_time);

	return sock_ctx;
}

void release_sock_ctx(sock_ctx_t *sock_ctx)
{
	fprintf(stderr, "%s() called sock from(%s:%d) fd(%d) (%.19s~) closed!\n",
			__func__, sock_ctx->client_ipaddr, sock_ctx->client_port, sock_ctx->sock_fd, ctime(&sock_ctx->create_time));

	event_del(sock_ctx->ev_flush_cb);
	close(sock_ctx->sock_fd);
	free(sock_ctx);
}

/* handle by main */
void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *data)
{
	fprintf(stderr, "{dbg} %s called! (main) (fd:%d)\n", __func__, fd);
	/* 1) check ue src ip */
	/* 2) get ue ctx */
	/* 3) get worker_ctx (ue_ctx) */
	/* 4) event worker, ue create bev */

	char client_ip[INET_ADDRSTRLEN] = {0,};
	sprintf(client_ip, "%s", util_get_ip_from_sa(sa));

	int ue_index = ipaddr_range_calc(MAIN_CTX->ue_info.ue_start_ip, client_ip);
	if (ue_index < 0 || ue_index >= MAIN_CTX->ue_info.ue_num) {
		fprintf(stderr, "%s() called with invalid client_ip=(%s)\n", __func__, client_ip);
		goto LC_ERR;
	}

	worker_ctx_t *worker_ctx = worker_ctx_get_by_index(ue_index);
	sock_ctx_t *sock_ctx = create_sock_ctx(fd, sa, ue_index);

	event_base_once(worker_ctx->evbase_thrd, -1, EV_TIMEOUT, accept_sock_cb, sock_ctx, NULL);

	return;
	
LC_ERR:
	close(fd);
}

/* handle by worker */
void accept_sock_cb(int conn_fd, short events, void *data)
{
	fprintf(stderr, "{dbg} %s called! (worker:%s)\n", __func__, WORKER_CTX->thread_name);

	sock_ctx_t *sock_ctx = (sock_ctx_t *)data; /* must be free */
	ue_ctx_t *ue_ctx = ue_ctx_get_by_index(sock_ctx->ue_index, WORKER_CTX);

	if (ue_ctx == NULL || ue_ctx->ipsec_sa_created == 0) {
		fprintf(stderr, "%s() ue_ctx(index:%d) not created or ipsec sa not exist!\n", __func__, sock_ctx->ue_index);
		goto ASC_ERR;
	}

	/* release older sock if exist */
	if (ue_ctx->sock_ctx != NULL) {
		release_sock_ctx(ue_ctx->sock_ctx);
		ue_ctx->sock_ctx = NULL;
	}
	/* accept new sock */
	accept_new_client(ue_ctx, sock_ctx);

	return;

ASC_ERR:
	close(sock_ctx->sock_fd);
	free(sock_ctx);
}

void accept_new_client(ue_ctx_t *ue_ctx, sock_ctx_t *sock_ctx)
{
	ue_ctx->sock_ctx = sock_ctx;

	sock_ctx->bev = bufferevent_socket_new(WORKER_CTX->evbase_thrd, sock_ctx->sock_fd, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS|BEV_OPT_THREADSAFE);
	sock_event_cb(sock_ctx->bev, BEV_EVENT_CONNECTED, ue_ctx);
	bufferevent_setcb(sock_ctx->bev, sock_read_cb, NULL, sock_event_cb, ue_ctx);
	bufferevent_enable(sock_ctx->bev, EV_READ);
}

void sock_event_cb(struct bufferevent *bev, short events, void *data)
{
    ue_ctx_t *ue_ctx = (ue_ctx_t *)data;
	sock_ctx_t *sock_ctx = ue_ctx->sock_ctx;

    if (events & BEV_EVENT_CONNECTED) {
        int fd = bufferevent_getfd(bev);
        util_set_linger(fd, 1, 0);
        sock_ctx->connected = 1;
        return;
    }

    if (events & BEV_EVENT_EOF) {
        fprintf(stderr, "%s() confirm closed!\n", __func__);
    } else if (events & BEV_EVENT_ERROR) {
        fprintf(stderr, "%s() got conn error!(%d:%s)\n", __func__, errno, strerror(errno));
    }

	release_sock_ctx(sock_ctx);
	ue_ctx->sock_ctx = NULL;
}

void sock_read_cb(struct bufferevent *bev, void *data)
{
	ue_ctx_t *ue_ctx = (ue_ctx_t *)data;
	sock_ctx_t *sock_ctx = ue_ctx->sock_ctx;

	unsigned char mem_buff[1024] = {0,};
	size_t recv_bytes = bufferevent_read(bev, mem_buff, sizeof(mem_buff));

	if (recv_bytes <= 0 || errno != EINTR) {
		fprintf(stderr, "%s() sock error! (%d:%s)\n", __func__, errno, strerror(errno));
		release_sock_ctx(sock_ctx);
		ue_ctx->sock_ctx = NULL;
		return;
	}

	char nas_str[2048 + 1] = {0,};
	mem_to_hex(mem_buff, recv_bytes, nas_str);

    return ngap_send_uplink_nas(ue_ctx, nas_str);
}
