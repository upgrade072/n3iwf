#include "sctp_c.h"

conn_curr_t *take_conn_list(main_ctx_t *MAIN_CTX)
{
	conn_curr_t *CURR_CONN = &MAIN_CTX->SHM_SCTPC_CONN->curr_conn[MAIN_CTX->SHM_SCTPC_CONN->curr_pos];

	return CURR_CONN;
}

int sort_conn_list(const void *a, const void *b)
{
	const conn_info_t *v_a = a;
	const conn_info_t *v_b = b;
	return (strcmp(v_a->name, v_b->name));
}

void disp_conn_list(main_ctx_t *MAIN_CTX)
{
	conn_curr_t *CURR_CONN = &MAIN_CTX->SHM_SCTPC_CONN->curr_conn[MAIN_CTX->SHM_SCTPC_CONN->curr_pos];
	for (int i = 0; i < CURR_CONN->rows_num; i++) {
		conn_info_t *conn = &CURR_CONN->dist_info[i];
		fprintf(stderr, "{dbg} %s id=(%d) name=(%s)\n", __func__, conn->id, conn->name);
	}
}

void init_conn_list(main_ctx_t *MAIN_CTX)
{
	config_setting_t *conf_conn_list = config_lookup(&MAIN_CTX->CFG, "connection_list");
	int conn_list_num = conf_conn_list == NULL ? 0 : config_setting_length(conf_conn_list);
	if (conn_list_num <= 0) {
		return;
	}

	int shm_prepare_pos = (MAIN_CTX->SHM_SCTPC_CONN->curr_pos + 1) % MAX_SC_CONN_SLOT;

	conn_curr_t *CURR_CONN = &MAIN_CTX->SHM_SCTPC_CONN->curr_conn[shm_prepare_pos];
	CURR_CONN->rows_num = 0;
	for (int i = 0; i < MAX_SC_CONN_LIST; i++) {
		CURR_CONN->conn_info[i].occupied = 0;
		CURR_CONN->dist_info[i].occupied = 0;
	}

	for (int i = 0; i < conn_list_num; i++) {
		config_setting_t *elem = config_setting_get_elem(conf_conn_list, i);

		/* prepare */
		int id = 0;
		const char *name = NULL;
		int enable = 0;
		int conn_num = 0;
		const char *src_ip_a = NULL;
		const char *src_ip_b = NULL;
		const char *src_ip_c = NULL;
		const char *dst_ip_a = NULL;
		const char *dst_ip_b = NULL;
		const char *dst_ip_c = NULL;
		int src_port = 0;
		int dst_port = 0;

		/* get from config */
		config_setting_lookup_int(elem, "id", &id);
		config_setting_lookup_string(elem, "name", &name);
		config_setting_lookup_int(elem, "enable", &enable);
		config_setting_lookup_int(elem, "conn_num", &conn_num);
		config_setting_lookup_string(elem, "src_ip_a", &src_ip_a);
		config_setting_lookup_string(elem, "src_ip_b", &src_ip_b);
		config_setting_lookup_string(elem, "src_ip_c", &src_ip_c);
		config_setting_lookup_string(elem, "dst_ip_a", &dst_ip_a);
		config_setting_lookup_string(elem, "dst_ip_b", &dst_ip_b);
		config_setting_lookup_string(elem, "dst_ip_c", &dst_ip_c);
		config_setting_lookup_int(elem, "src_port", &src_port);
		config_setting_lookup_int(elem, "dst_port", &dst_port);

		/* check value */
		if (id < 0 || id >= MAX_SC_CONN_LIST) {
			fprintf(stderr, "%s() check invalid id=(%d)\n", __func__, id);
			continue;
		} else if (CURR_CONN->conn_info[id].occupied == 1) {
			fprintf(stderr, "%s() check duplicated id=(%d)\n", __func__, id);
			continue;
		} else if (strlen(name) == 0 || strlen(name) >= 128) {
			fprintf(stderr, "%s() check invalid name_len(%ld) id=(%d)\n", __func__, strlen(name), id);
			continue;
		} else if (conn_num < 0 || conn_num >= MAX_SC_CONN_NUM) {
			fprintf(stderr, "%s() check invalid conn_num(%d) id=(%d)\n", __func__, conn_num, id);
			continue;
		}

		/* set to shm */
		conn_info_t *conn = &CURR_CONN->conn_info[id];
		conn->occupied = 1;
		conn->id = id; 
		sprintf(conn->name, "%.127s", name);
		conn->enable = enable;
		conn->conn_num = conn_num;
		sprintf(conn->src_addr[0], "%.127s", src_ip_a);
		sprintf(conn->src_addr[1], "%.127s", src_ip_b);
		sprintf(conn->src_addr[2], "%.127s", src_ip_c);
		sprintf(conn->dst_addr[0], "%.127s", dst_ip_a);
		sprintf(conn->dst_addr[1], "%.127s", dst_ip_b);
		sprintf(conn->dst_addr[2], "%.127s", dst_ip_c);
		conn->sport = src_port;
		conn->dport = dst_port;
		memset(conn->conn_status, 0x00, sizeof(conn_status_t) * MAX_SC_CONN_NUM);

		/* dist_info will sorting by name */
		memcpy(&CURR_CONN->dist_info[CURR_CONN->rows_num], conn, sizeof(conn_info_t));
		CURR_CONN->rows_num++;
	}

	/* sort for bf_thread */
	qsort(CURR_CONN->dist_info, CURR_CONN->rows_num, sizeof(conn_info_t), sort_conn_list);

	/* publish to shm with <curr_pos> */
	MAIN_CTX->SHM_SCTPC_CONN->curr_pos = shm_prepare_pos;
}
