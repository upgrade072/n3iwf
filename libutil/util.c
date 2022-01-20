#include "libutil.h"

void util_shell_cmd_apply(char *command, char *res, size_t res_size)
{
    FILE *p = popen(command, "r");
    if (p != NULL) {
        if (fgets(res, res_size, p) == NULL)
            res = NULL;
        pclose(p);
    }
}

void util_print_msgq_info(int key, int msqid)
{
	struct msqid_ds m_stat = {0,};

	fprintf(stderr, "---------- messege queue info -------------\n");
	if (msgctl(msqid, IPC_STAT, &m_stat)== -1) {
		fprintf(stderr, "msgctl failed");
		return;
	}
	fprintf(stderr, " queue key : 0x%x\n", key);
	fprintf(stderr, " msg_lspid : %d\n",  m_stat.msg_lspid);
	fprintf(stderr, " msg_qnum  : %lu\n", m_stat.msg_qnum);
	fprintf(stderr, " msg_stime : %lu\n", m_stat.msg_stime);
	fprintf(stderr, "-------------------------------------------\n");
}
