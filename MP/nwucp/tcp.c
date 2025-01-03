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
	sock_ctx->remain_size = 0;

	return sock_ctx;
}

void release_sock_ctx(sock_ctx_t *sock_ctx)
{
	ERRLOG(LLE, FL, "%s() called sock from(%s:%d) fd(%d) (%.19s~) closed!\n",
			__func__, sock_ctx->client_ipaddr, sock_ctx->client_port, sock_ctx->sock_fd, ctime(&sock_ctx->create_time));

	if (sock_ctx->bev) {
		bufferevent_free(sock_ctx->bev);
		sock_ctx->bev = NULL;
	}

	close(sock_ctx->sock_fd);
	free(sock_ctx);
}

/* handle by main */
void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *data)
{
	/* 1) check ue src ip */
	/* 2) get ue ctx */
	/* 3) get worker_ctx (ue_ctx) */
	/* 4) event worker, ue create bev */

	char client_ip[INET_ADDRSTRLEN] = {0,};
	sprintf(client_ip, "%s", util_get_ip_from_sa(sa));

	ERRLOG(LLE, FL, "%s() called! (main) (from:%s) (fd:%d)\n", __func__, client_ip, fd);

	int ue_index = ipaddr_range_calc(MAIN_CTX->ue_info.ue_start_ip, client_ip);
	if (ue_index < 0 || ue_index >= MAIN_CTX->ue_info.ue_num) {
		ERRLOG(LLE, FL, "%s() called with invalid client_ip=(%s)\n", __func__, client_ip);
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
	ERRLOG(LLE, FL, "%s() called! (at worker:%s)\n", __func__, WORKER_CTX->thread_name);

	sock_ctx_t *sock_ctx = (sock_ctx_t *)data; /* must be free */
	ue_ctx_t *ue_ctx = ue_ctx_get_by_index(sock_ctx->ue_index, WORKER_CTX);

#if 0
	if (ue_ctx == NULL || ue_ctx->ipsec_sa_created == 0) {
		ERRLOG(LLE, FL, "%s() ue_ctx(index:%d) not created or ipsec sa not exist!\n", __func__, sock_ctx->ue_index);
		goto ASC_ERR;
#else
	if (ue_ctx == NULL) {
		ERRLOG(LLE, FL, "%s() ue_ctx (ip:%s/index:%d) not created!\n", __func__, sock_ctx->client_ipaddr, sock_ctx->ue_index);
		goto ASC_ERR;
#endif
	}

	/* release older sock if exist */
	if (ue_ctx->sock_ctx != NULL) {
		ERRLOG(LLE, FL, "%s() check ue [%s] have old sock_ctx, release!\n", __func__, UE_TAG(ue_ctx));
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

	sock_flush_cb(ue_ctx);
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
        ERRLOG(LLE, FL, "%s() confirm closed!\n", __func__);
    } else if (events & BEV_EVENT_ERROR) {
        ERRLOG(LLE, FL, "%s() got conn error!(%d:%s)\n", __func__, errno, strerror(errno));
    }

	release_sock_ctx(sock_ctx);
	ue_ctx->sock_ctx = NULL;
}

void sock_read_cb(struct bufferevent *bev, void *data)
{
	ue_ctx_t *ue_ctx = (ue_ctx_t *)data;
	sock_ctx_t *sock_ctx = ue_ctx->sock_ctx;

	size_t read_maximum = SOCK_BUFF_MAX_SIZE - sock_ctx->remain_size;
	size_t read_bytes = 0;

SRC_RETRY:
	read_bytes = bufferevent_read(bev, sock_ctx->sock_buffer + sock_ctx->remain_size, read_maximum);
	sock_ctx->remain_size += read_bytes;

	nas_envelop_t *nas = (nas_envelop_t *)sock_ctx->sock_buffer;
	while (sock_ctx->remain_size >= sizeof(nas_envelop_t)) {
		size_t nas_size = sizeof(uint16_t) + ntohs(nas->length);
		if (nas_size <= sock_ctx->remain_size) {
			char nas_str[10240] = {0,};
			nas->length = ntohs(nas->length);
			mem_to_hex((unsigned char *)nas->message, nas->length, nas_str);

			NWUCP_TRACE(ue_ctx, DIR_UE_TO_ME, NULL, nas_str);

			if (NWUCP_OVLD_CHK_TCP() == 0) {
				if (ngap_send_uplink_nas(ue_ctx, nas_str) < 0) {
				}
			}
			memmove(sock_ctx->sock_buffer, &sock_ctx->sock_buffer[nas_size], sock_ctx->remain_size - nas_size);
			sock_ctx->remain_size -= nas_size;
		}
	}

	if (read_bytes == read_maximum) {
		goto SRC_RETRY; /* maybe data remain */
	}
	if (errno == EINTR) {
		ERRLOG(LLE, FL, "%s() ue [%s] sock error! (%d:%s)\n", __func__, UE_TAG(ue_ctx), errno, strerror(errno));
		release_sock_ctx(sock_ctx);
		ue_ctx->sock_ctx = NULL;
		return;
	} 
}

void sock_flush_cb(ue_ctx_t *ue_ctx)
{
	sock_ctx_t *sock_ctx = ue_ctx->sock_ctx;
	if (sock_ctx == NULL) {
		ERRLOG(LLE, FL, "TODO %s() fail cause sock_ctx=[null]!\n", __func__);
		return;
	}

	char mem_buff[10240] = {0,};
	nas_envelop_t *nas = (nas_envelop_t *)mem_buff;

	while (ue_ctx->temp_cached_nas_message && ue_ctx->temp_cached_nas_message->data) {

		NWUCP_TRACE(ue_ctx, DIR_ME_TO_UE, NULL, ue_ctx->temp_cached_nas_message->data);

		size_t nas_len = 0;
		hex_to_mem(ue_ctx->temp_cached_nas_message->data, nas->message, &nas_len);
		nas->length = htons(nas_len);

		struct iovec iov[2];
		iov[0].iov_base = &nas->length;
		iov[0].iov_len = sizeof(uint16_t);
		iov[1].iov_base = nas->message;
		iov[1].iov_len = nas_len;

		size_t expect_len = iov[0].iov_len + iov[1].iov_len;
		if (writev(sock_ctx->sock_fd, iov, 2) != expect_len) {
			ERRLOG(LLE, FL, "%s() ue [%s] writev failed (%d:%s)\n", __func__, UE_TAG(ue_ctx), errno, strerror(errno));
			release_sock_ctx(sock_ctx);
			ue_ctx->sock_ctx = NULL;
			return;
		}

		free(ue_ctx->temp_cached_nas_message->data);
		ue_ctx->temp_cached_nas_message = g_slist_remove(ue_ctx->temp_cached_nas_message, ue_ctx->temp_cached_nas_message->data);
	}
}

void sock_flush_temp_nas_pdu(ue_ctx_t *ue_ctx)
{
	while (ue_ctx->temp_cached_nas_message && ue_ctx->temp_cached_nas_message->data) {
		free(ue_ctx->temp_cached_nas_message->data);
		ue_ctx->temp_cached_nas_message = g_slist_remove(ue_ctx->temp_cached_nas_message, ue_ctx->temp_cached_nas_message->data);
	}
}
