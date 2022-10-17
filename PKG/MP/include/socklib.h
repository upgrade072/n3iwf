#ifndef __SOCKLIB_H__
#define __SOCKLIB_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
//#include <stropts.h>
#include <string.h>
#include <fcntl.h>
#include <sys/errno.h>
#include "trclib.h"

#define SOCKLIB_MAX_BIND_CNT			4
#define SOCKLIB_MAX_SERVER_CNT			64
#define SOCKLIB_MAX_CLIENT_CNT			256

#define SOCKLIB_POLL_TIMER				1000
#define SOCKLIB_SEG_BUF_SIZE			(1024*256)
#define SOCKLIB_RETRY_CNT				100
#define SOCKLIB_RETRY_WAIT_TIMER		10000
#define SOCKLIB_PRINT_TO        		stdout
//#define SOCKLIB_RETRY_WAIT_TIMER		1000
#define SOCKLIB_RCV_BUF_SIZE			(1024*32)

#define SOCKLIB_NO_EVENT				0
#define SOCKLIB_NEW_CONNECTION			1
#define SOCKLIB_CLIENT_MSG_RECEIVED		2
#define SOCKLIB_SERVER_MSG_RECEIVED		3
#define SOCKLIB_CLIENT_DISCONNECTED		4
#define SOCKLIB_SERVER_DISCONNECTED		5
#define SOCKLIB_INTERNAL_ERROR			6


/* 메시지 큐 통신에 사용되는 mtype이 long으로 선언되어야 하는데, 시스템에 따라 long이
//  4byte 또는 8byte로 다르다.
// - 이를 위해 ixpc에서 msgQ로 받은 메시지 중 mtype을 떼어내 socket_header에 4byte로
//  넣어서 보내고 수신측에서 다시 떼어 붙여서 msgQ로 보낸다.
*/
typedef struct {
	int		bodyLen;
    int     mapType;	/* jumaeng 2011.04.11 for rmi */
    int     mtype;   /* 시스템에 따라 long이 4byte 또는 8byte로 다르다. */
} SockLibHeadType;

#define SOCKLIB_HEAD_LEN		sizeof(SockLibHeadType)
//#define SOCKLIB_MAX_BODY_LEN	(COMM_MAX_IPCBUF_LEN)-(SOCKLIB_HEAD_LEN)
#define SOCKLIB_MAX_BODY_LEN	(16384)-(SOCKLIB_HEAD_LEN) /* JKLEE 2011/03/28 */
typedef struct {
	SockLibHeadType	head;
	char			body[SOCKLIB_MAX_BODY_LEN];
} SockLibMsgType;



typedef struct {
	int			fd;
	int			port;
} BindSockFdContext;

/*
*  ServerSockFdContext와 ClientSockFdContext의 structure는 같다.
*/
typedef struct {
	int					fd;
	struct sockaddr_in	rmtAddr;		// 접속한 서버의 ip_address or 접속해온 client의 ip_address
	int					port;			// 접속한 서버의 port 번호 or 내가 bind한 어떤 port로 접속해 왔는지 binding port 번호
	time_t				connectTime;	// 접속된 시각
	time_t				lastTxTime;		// 마지막으로 데이터를 전송한 시각
	time_t				lastRxTime;		// 마지막으로 데이터를 수신한 시각
	int					rcvMsgLen;		// rcvBuf에 쌓여있는 메시지 길이
	unsigned char		rcvBuf[SOCKLIB_RCV_BUF_SIZE]; // 해당 fd에서 수신된 메시지를 넣어두는 buffer
} SockLibSockFdContext;


/* add jumaeng 2011.04.11 */
typedef struct {
    int         fd;
    int         port;
    struct sockaddr_in  srvAddr;
    time_t      tStamp;
} ServerSockFdContext;
 
typedef struct {
    int         fd;
    int         port;
    struct sockaddr_in  cliAddr;
    time_t      tStamp;
} ClientSockFdContext;


//extern int errno;

extern int socklib_setAllowClientAddrTbl (char*);
extern int socklib_initTcpBind (int);
extern int socklib_connect (char*, int);
extern int socklib_connect_bind (char*, int, char*, int);
extern int socklib_connect_noALRM (char*, int);
extern int socklib_acceptNewConnection (int, int, int*);
extern int socklib_disconnectSockFd (int);
extern int socklib_disconnectServerFd (int);
extern int socklib_disconnectClientFd (int);
extern int socklib_action (char*, int*);
extern int socklib_readSockFd (char*, int);
extern int socklib_sndMsg (int, char*, int);
extern int socklib_broadcast (char*, int);
extern int socklib_broadcast2Servers (char*, int);
extern int socklib_broadcast2Clients (char*, int);
extern int socklib_setSockOption (int);
extern int socklib_setNonBlockMode (int);
extern int socklib_addBindSockFdTbl (int, int);
extern int socklib_addServerSockFdTbl (struct sockaddr_in *, int, int);
extern int socklib_addClientSockFdTbl (struct sockaddr_in *, int, int);
extern int socklib_delServerSockFdTbl (int);
extern int socklib_delClientSockFdTbl (int);
extern int socklib_addSockFdSet (int);
extern int socklib_delSockFdSet (int);
extern int socklib_pollFdSet (void);
extern int socklib_lookupBindSockFdTbl (int*, int*);
extern int socklib_lookupServerSockFdTbl (int*);
extern int socklib_lookupClientSockFdTbl (int*);

extern int socklib_action_2 (int*, int*);
extern int socklib_readSockFd_2 (SockLibSockFdContext*);
extern int socklib_sndMsg_2 (int, char*, int);
extern int socklib_connect_2 (char*, int, int*);
extern int socklib_connect_noALRM_2 (char*, int, int*);
extern int socklib_connect_bind_2 (char*, int, char*, int, int*);

#endif /*__SOCKLIB_H__*/
