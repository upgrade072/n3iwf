#include "ngapp.h"

extern main_ctx_t *MAIN_CTX;
static __thread worker_ctx_t *WORKER_CTX;

void *io_worker_thread(void *arg)
{
	worker_ctx_t *worker_ctx = (worker_ctx_t *)arg;
	fprintf(stderr, "{dbg} %s=[%d:%s]\n", __func__, worker_ctx->thread_index, worker_ctx->thread_name);
	return NULL;
}

