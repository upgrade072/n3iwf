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
		fprintf(stderr, "msgctl failed (key=0x%x:msgqid=%d) [%d:%s]\n", key, msqid, errno, strerror(errno));
		return;
	}
	fprintf(stderr, " queue key : 0x%x\n", key);
	fprintf(stderr, " msg_lspid : %d\n",  m_stat.msg_lspid);
	fprintf(stderr, " msg_qnum  : %lu\n", m_stat.msg_qnum);
	fprintf(stderr, " msg_stime : %lu\n", m_stat.msg_stime);
	fprintf(stderr, "-------------------------------------------\n");
}

int util_get_queue_info(int key, const char *prefix)
{
	int msgq_id = 0;

	if (key <= 0 || ((msgq_id = msgget(key, IPC_CREAT | 0666)) < 0)) {
		fprintf(stderr, "%s() fail to create msgq [%s:key=(%d)]\n", __func__, prefix, key);
		return -1;
	}

	fprintf(stderr, "msgq_id=(%d)\n", msgq_id);
	util_print_msgq_info(key, msgq_id);
	return msgq_id;
}

void *util_get_shm(int key, size_t size, const char *prefix)
{
	int shm_id = 0;

	if (key <= 0 || ((shm_id = shmget(key, size, IPC_CREAT|0666)) < 0)) {
		fprintf(stderr, "%s() fail to create shm [%s:key=(%d)]\n", __func__, prefix, key);
		return NULL;
	}

	return shmat(shm_id, NULL, 0);
}

// return ip address 
char *util_get_ip_from_sa(struct sockaddr *sa)
{
    struct sockaddr_in *peer_addr = (struct sockaddr_in *)sa;
    return inet_ntoa(peer_addr->sin_addr);
}

// return port no
int util_get_port_from_sa(struct sockaddr *sa)
{
    in_port_t port = {0,};

    if (sa->sa_family == AF_INET) {
        port = (((struct sockaddr_in*)sa)->sin_port);
    } else {
        port = (((struct sockaddr_in6*)sa)->sin6_port);
    }

    return ntohs(port);
}

// set so_linger (abort) option to sock
int util_set_linger(int fd, int onoff, int linger)
{
    struct linger l = { .l_linger = linger, .l_onoff = onoff};
    int res = setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l));

    return res;
}

int util_set_rcvbuffsize(int fd, int byte)
{
    int opt = byte;
    int res = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt));

    return res;
}

int util_set_sndbuffsize(int fd, int byte)
{
    int opt = byte;
    int res = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt));

    return res;
}

// set keepalived option to sock
int util_set_keepalive(int fd, int keepalive, int cnt, int idle, int intvl)
{
    int res = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
    res = setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &idle, sizeof(idle));
    res = setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    res = setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl));

    return res;
}