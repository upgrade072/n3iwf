#include <eapp.h>

extern main_ctx_t *MAIN_CTX;

recv_buf_t *get_recv_buf(recv_buff_t *recv_buff)
{
	for (int i = 0; i < recv_buff->total_num; i++) {
		int buff_index = (recv_buff->curr_index++) % recv_buff->total_num;
		recv_buf_t *recv_buf = &recv_buff->buffers[buff_index];
		if (recv_buf->occupied == 0){
			/* set to use */
			recv_buf->occupied = 1;
			recv_buf->size = 0;
			recv_buff->total_num++;
			return recv_buf;
		}
	}
	return NULL;
}

void release_recv_buf(recv_buff_t *recv_buff, recv_buf_t *recv_buf)
{
	recv_buff->total_num--;
	recv_buf->size = 0;
	recv_buf->occupied = 0;
}
