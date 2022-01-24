#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/queue.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <arpa/inet.h>

/* ------------------------- util.c --------------------------- */
void    util_shell_cmd_apply(char *command, char *res, size_t res_size);
void    util_print_msgq_info(int key, int msqid);
int     util_get_queue_info(int key, const char *prefix);
void    *util_get_shm(int key, size_t size, const char *prefix);
char    *util_get_ip_from_sa(struct sockaddr *sa);
int     util_get_port_from_sa(struct sockaddr *sa);
int     util_set_linger(int fd, int onoff, int linger);
int     util_set_rcvbuffsize(int fd, int byte);
int     util_set_sndbuffsize(int fd, int byte);
int     util_set_keepalive(int fd, int keepalive, int cnt, int idle, int intvl);
