#include "sctpc.h"

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

	ft_table_t *table = ft_create_table();
	ft_set_border_style(table, FT_PLAIN_STYLE);
	ft_add_separator(table);
	ft_write_ln(table, "[config]", "[connection]", "[association]");
	ft_add_separator(table);

	//for (int i = 0; i < CURR_CONN->rows_num; i++) {
	for (int i = 0; i < MAX_SC_CONN_LIST; i++) {
		conn_info_t *conn = &CURR_CONN->conn_info[i];
		if (!conn->occupied) continue;

		ft_table_t *assoc_table = ft_create_table();
		ft_set_border_style(assoc_table, FT_PLAIN_STYLE);
		ft_write_ln(assoc_table, "conns_fd", "assoc_id", "assoc_state");
		ft_add_separator(assoc_table);

		for (int k = 0; k < conn->conn_num; k++) {
			conn_status_t *conn_status = &conn->conn_status[k];
			const char *sctp_state = NULL;
			get_assoc_state(conn_status->conns_fd, conn_status->assoc_id, &sctp_state);
			ft_printf_ln(assoc_table, "%d|%d|%s", conn_status->conns_fd, conn_status->assoc_id, sctp_state);
		}

		ft_printf_ln(table, "occupied=(%d)\nid=(%d)\nname=(%s)\nenable=(%d)\nconn_num=(%d)|src_1=(%s)\nsrc_2=(%s)\nsrc_3=(%s)\ndst_1=(%s)\ndst_2=(%s)\ndst_3=(%s)\nsport=(%d)\ndport=(%d)|%s",
				conn->occupied,
				conn->id,
				conn->name,
				conn->enable,
				conn->conn_num,
				conn->src_addr[0], conn->src_addr[1], conn->src_addr[2], conn->dst_addr[0], conn->dst_addr[1], conn->dst_addr[2],
				conn->sport,
				conn->dport,
				ft_to_string(assoc_table));
		ft_add_separator(table);
		ft_destroy_table(assoc_table);
	}
	ft_add_separator(table);
	fprintf(stderr, "%s\n", ft_to_string(table));
	ft_destroy_table(table);
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

void disp_conn_stat(main_ctx_t *MAIN_CTX)
{
	conn_curr_t *CURR_CONN = &MAIN_CTX->SHM_SCTPC_CONN->curr_conn[MAIN_CTX->SHM_SCTPC_CONN->curr_pos];

	/* prepare */
	for (int i = 0; i < link_node_length(&MAIN_CTX->sctp_stat_list); i++) {
		sctp_stat_t *sctp_stat = link_node_get_nth_data(&MAIN_CTX->sctp_stat_list, i);
		sctp_stat->used = 0;
	}

	/* calc & sum assoc stats */
	for (int i = 0; i < MAX_SC_CONN_LIST; i++) {
		conn_info_t *conn = &CURR_CONN->conn_info[i];
		if (!conn->occupied) continue;

		link_node *STAT_NODE = link_node_assign_key_order(&MAIN_CTX->SCTP_STAT, conn->name, sizeof(struct sctp_assoc_stats));
		struct sctp_assoc_stats *HOST_STAT = (struct sctp_assoc_stats *)STAT_NODE->data;

		for (int k = 0; k < conn->conn_num; k++) {
			conn_status_t *conn_status = &conn->conn_status[k];

			/* create key */
			char temp_key[128] = {0,}; sprintf(temp_key, "%d", conn_status->assoc_id);

			struct sctp_stat_t *sctp_stat = link_node_get_data_by_key(&MAIN_CTX->sctp_stat_list, temp_key);
			if (sctp_stat == NULL) {
				fprintf(stderr, "{dbg} create new sctp stat for [%s]\n", temp_key);
				link_node *node = link_node_assign_key_order(&MAIN_CTX->sctp_stat_list, temp_key, sizeof(sctp_stat_t));
				sctp_stat = (sctp_stat_t *)node->data;
			}
			get_assoc_stats_diff(conn_status->conns_fd, conn_status->assoc_id, sctp_stat);
			sum_assoc_stats(&sctp_stat->curr_stat, HOST_STAT);
		}
	}

	/* remove unused node */
	for (int i = link_node_length(&MAIN_CTX->sctp_stat_list) - 1; i >= 0; i--) {
		link_node *node = link_node_get_nth(&MAIN_CTX->sctp_stat_list, i);
		sctp_stat_t *sctp_stat = (sctp_stat_t *)node->data;
		if (sctp_stat->used == 0) {
			link_node_delete(&MAIN_CTX->sctp_stat_list, node);
		}
	}
}

void send_conn_stat(main_ctx_t *MAIN_CTX, IxpcQMsgType *rxIxpcMsg)
{
	int len = sizeof(int), txLen = 0;   
    int statCnt = 0;
            
    GeneralQMsgType sxGenQMsg;
    memset(&sxGenQMsg, 0x00, sizeof(GeneralQMsgType));
        
    IxpcQMsgType *sxIxpcMsg = (IxpcQMsgType*)sxGenQMsg.body;
    
    STM_CommonStatMsgType *commStatMsg=(STM_CommonStatMsgType *)sxIxpcMsg->body;
    STM_CommonStatMsg     *commStatItem=NULL;
    
    sxGenQMsg.mtype = MTYPE_STATISTICS_REPORT;
    sxIxpcMsg->head.msgId = MSGID_SCTP_STATISTIC_REPORT;
    sxIxpcMsg->head.seqNo = 0; // start from 1
            
    strcpy(sxIxpcMsg->head.srcSysName, rxIxpcMsg->head.dstSysName);
    strcpy(sxIxpcMsg->head.srcAppName, rxIxpcMsg->head.dstAppName);
    strcpy(sxIxpcMsg->head.dstSysName, rxIxpcMsg->head.srcSysName);
    strcpy(sxIxpcMsg->head.dstAppName, rxIxpcMsg->head.srcAppName);

	for (int i = 0; i < link_node_length(&MAIN_CTX->SCTP_STAT); i++) {
		link_node *STAT_NODE = link_node_get_nth(&MAIN_CTX->SCTP_STAT, i);
		struct sctp_assoc_stats *HOST_STAT = (struct sctp_assoc_stats *)STAT_NODE->data;

		print_assoc_stats(STAT_NODE->key, HOST_STAT);

		commStatItem = &commStatMsg->info[statCnt]; ++statCnt;
		memset(commStatItem, 0x00, sizeof(STM_CommonStatMsg));
		len += sizeof (STM_CommonStatMsg);

		snprintf(commStatItem->strkey1, sizeof(commStatItem->strkey1), "%s", STAT_NODE->key);

		int index = 0;
		commStatItem->ldata[index++] = HOST_STAT->sas_maxrto;
		commStatItem->ldata[index++] = HOST_STAT->sas_isacks;
		commStatItem->ldata[index++] = HOST_STAT->sas_osacks;
		commStatItem->ldata[index++] = HOST_STAT->sas_opackets;
		commStatItem->ldata[index++] = HOST_STAT->sas_ipackets;
		commStatItem->ldata[index++] = HOST_STAT->sas_rtxchunks;
		commStatItem->ldata[index++] = HOST_STAT->sas_outofseqtsns;
		commStatItem->ldata[index++] = HOST_STAT->sas_idupchunks;
		commStatItem->ldata[index++] = HOST_STAT->sas_gapcnt;
		commStatItem->ldata[index++] = HOST_STAT->sas_ouodchunks;
		commStatItem->ldata[index++] = HOST_STAT->sas_iuodchunks;
		commStatItem->ldata[index++] = HOST_STAT->sas_oodchunks;
		commStatItem->ldata[index++] = HOST_STAT->sas_iodchunks;
		commStatItem->ldata[index++] = HOST_STAT->sas_octrlchunks;
		commStatItem->ldata[index++] = HOST_STAT->sas_ictrlchunks;
	}
	commStatMsg->num = statCnt;
	ERRLOG(LLE, FL, "[SCTP / STAT Write Succeed stat_count=(%d) sedNo=(%d) body/txLen=(%d:%d)]\n",
			statCnt, sxIxpcMsg->head.seqNo, len, txLen);

	sxIxpcMsg->head.segFlag = 0;
	sxIxpcMsg->head.seqNo++;
	sxIxpcMsg->head.bodyLen = len;
	txLen = sizeof(sxIxpcMsg->head) + sxIxpcMsg->head.bodyLen;

	if (msgsnd(MAIN_CTX->QID_INFO.ixpc_qid, (void*)&sxGenQMsg, txLen, IPC_NOWAIT) < 0) {
		ERRLOG(LLE, FL, "DBG] %s() status send fail IXPC qid[%d] err[%s]\n", __func__, MAIN_CTX->QID_INFO.ixpc_qid, strerror(errno));
	}

	/* clear STAT */
	link_node_delete_all(&MAIN_CTX->SCTP_STAT, NULL);
}
