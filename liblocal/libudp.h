#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

/* ------------------------- libudp.c --------------------------- */
int     create_udp_sock(int port);
int     udpfromto_init(int s);
int     recvfromto(int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen, struct sockaddr *to, socklen_t *tolen);
int     sendfromto(int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t fromlen, struct sockaddr *to, socklen_t tolen);
int     check_v4_mapped_v6(struct sockaddr *addr);
