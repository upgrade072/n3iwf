#include <sys/time.h>
#include "socklib.h"

int 	maxSockFdNum=0;
fd_set	readSockFdSet, writeSockFdSet, exceptSockFdSet;
fd_set	rdSockFdSet, wrSockFdSet, exSockFdSet;
BindSockFdContext		bindSockFdTbl[SOCKLIB_MAX_BIND_CNT];
SockLibSockFdContext	serverSockFdTbl[SOCKLIB_MAX_SERVER_CNT];
SockLibSockFdContext	clientSockFdTbl[SOCKLIB_MAX_CLIENT_CNT];
in_addr_t	allowClientAddrTbl[SOCKLIB_MAX_CLIENT_CNT];
int         allowClientCnt=0;
int			socklib_connect_timer=1;
extern int	trcErrLogId;

/*------------------------------------------------------------------------------
* socklib에서 선언된 global variable을 초기화 한다.
------------------------------------------------------------------------------*/
void socklib_initial (char *fname)
{
	(void)fname;	/* remove compile time warning */
	FD_ZERO(&readSockFdSet);
	FD_ZERO(&writeSockFdSet);
	FD_ZERO(&exceptSockFdSet);
	memset ((void*)bindSockFdTbl, 0, sizeof(bindSockFdTbl));
	memset ((void*)serverSockFdTbl, 0, sizeof(serverSockFdTbl));
	memset ((void*)clientSockFdTbl, 0, sizeof(clientSockFdTbl));
	memset ((void*)allowClientAddrTbl, 0, sizeof(allowClientAddrTbl));

	return;

} /** End of socklib_initial **/


/*------------------------------------------------------------------------------
* 지정된 file을 읽어 allowClientAddrTbl을 setting한다.
* - client로부터 connect 요구가 수신 되었을때 여기에 기록된 address만 허용한다.
------------------------------------------------------------------------------*/
int socklib_setAllowClientAddrTbl (char *fname)
{
	FILE	*fp;
	char	getBuf[256], token[4][64];
	int		allowCnt=0;

	if ((fp = fopen(fname,"r")) == NULL) {
		fprintf(stderr,"[socklib_setAllowClientAddrTbl] fopen fail[%s]; errno=%d(%s)\n",fname,errno,strerror(errno));
		return -1;
	}

	while (fgets(getBuf,sizeof(getBuf),fp) != NULL) {
		if (getBuf[0]=='#' || getBuf[0]=='\n')
			continue;
		sscanf (getBuf, "%63s", token[0]);
		allowClientAddrTbl[allowCnt++] = inet_addr(token[0]);
		if (allowCnt >= SOCKLIB_MAX_CLIENT_CNT)
			break;
	}
	fclose(fp);

	allowClientCnt = allowCnt;

	return allowCnt;

} /** End of socklib_setAllowClientAddrTbl **/



/*------------------------------------------------------------------------------
* 지정된 port로 socket fd를 생성하고, bind/listen한다.
* - 생성된 fd를 read/except fd_set에 추가한다.
* - 어떤 binding port에 connect 요구가 감지되었는지 확인하기 위해 생성된 fd를
*	bindSockFdTbl에 추가한다.
*	-> 즉, 여러개의 bindin port를 생성/관리할 수 있다.
* - binding을 INADDR_ANY로 함으로써, 시스템에 설정된 모든 ip_address에 대해
*	binding 된다.
* - 생성된 fd가 return된다.
------------------------------------------------------------------------------*/
int socklib_initTcpBind (int port)
{
	int		fd;
	struct sockaddr_in	myAddr;

	/* create socket fd
	*/
	if ((fd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr,"[socklib_initTcpBind] socket fail; errno=%d(%s)\n",errno,strerror(errno));
		return -1;
	}

	/*
	*/
	if (socklib_setSockOption(fd) < 0) {
		close(fd);
		return -1;
	}
	if (socklib_setNonBlockMode(fd) < 0) {
		close(fd);
		return -1;
	}

	/* bind fd
	*/
	memset ((void*)&myAddr, 0, sizeof(myAddr));
	myAddr.sin_family = AF_INET;
	myAddr.sin_port = htons((u_short)port);
	myAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(fd, (struct sockaddr*)&myAddr, sizeof(myAddr)) < 0 ) {
		fprintf(stderr,"[socklib_initTcpBind] bind fail; errno=%d(%s)\n",errno,strerror(errno));
		close(fd);
		return -1;
	}

	/* listen
	*/
	if (listen(fd, SOMAXCONN) < 0) {
		fprintf(stderr,"[socklib_initTcpBind] listen fail; errno=%d(%s)\n",errno,strerror(errno));
		close(fd);
		return -1;
	}

	/* bindSockFdTbl에 port와 fd를 저장한다.
	*/
	if (socklib_addBindSockFdTbl (port, fd) < 0) {
		close(fd);
		return -1;
	}

	/* add socket fd to readSockFdSet, exceptSockFdSet
	*/
	socklib_addSockFdSet(fd); 

	return fd;

} /** End of socklib_initTcpBind **/



void socklib_dumHdlr (int signo) { (void)signo; /* remove compile time warning */}

/*------------------------------------------------------------------------------
* 지정된 ipAddr,port로 socket fd를 생성하여 접속한다.
* - 생성된 fd를 read/except fd_set에 추가한다.
* - 생성된 fd가 return된다.
------------------------------------------------------------------------------*/
int socklib_connect (char *ipAddr, int port)
{
	int		fd, fdTbl_index;
	struct sockaddr_in	dstAddr;

	if (!strcmp(ipAddr,""))
		return -1;

	/* create socket fd
	*/
	if ((fd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr,"[socklib_connect] socket fail; errno=%d(%s)\n",errno,strerror(errno));
		return -1;
	}

	/*
	*/
	if (socklib_setSockOption(fd) < 0) {
		close(fd);
		return -1;
	}

	/* connect to server
	*/
	memset ((void*)&dstAddr, 0, sizeof(dstAddr));
	dstAddr.sin_family = AF_INET;
	dstAddr.sin_port = htons((u_short)port);
	dstAddr.sin_addr.s_addr = inet_addr(ipAddr);

	signal (SIGALRM, socklib_dumHdlr);
	//ualarm ((useconds_t)(100000*socklib_connect_timer), 0); // --->> multi-thread인 경우 문제가 될수 있다.
	ualarm (100000*socklib_connect_timer, 0); // --->> multi-thread인 경우 문제가 될수 있다.
	if (connect(fd, (struct sockaddr*)&dstAddr, sizeof(dstAddr)) < 0 ) {
#ifdef DEBUG
		//fprintf(stdout,"[socklib_connect] connect fail[%s %d]; errno=%d(%s)\n",ipAddr,port,errno,strerror(errno));
#endif
		ualarm(0,0);
		close(fd);
		return -1;
	}
	ualarm(0,0);

	if (socklib_setNonBlockMode(fd) < 0) {
		close(fd);
		return -1;
	}

	/* serverSockFdTbl에 address, port, fd를 저장한다.
	*/
	fdTbl_index = socklib_addServerSockFdTbl (&dstAddr, port, fd);
	if (fdTbl_index < 0) {
		close(fd);
		return -1;
	}

	/* add socket fd to readSockFdSet, exceptSockFdSet
	*/
	socklib_addSockFdSet(fd);

	return fd;

} /** End of socklib_connect **/



/*------------------------------------------------------------------------------
* 지정된 srcIpAddr,port를 갖고, dstIp,port로 socket fd를 생성하여 접속한다.
* - 생성된 fd를 read/except fd_set에 추가한다.
* - 생성된 fd가 return된다.
------------------------------------------------------------------------------*/
int socklib_connect_bind (char *dstIp, int dstPort, char *srcIp, int srcPort)
{
	int		fd, fdTbl_index;
	struct sockaddr_in	dstAddr;
	struct sockaddr_in	srcAddr;

	if (!strcmp(dstIp,""))
		return -1;

	/* create socket fd
	*/
	if ((fd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr,"[socklib_connect_bind] socket fail; errno=%d(%s)\n",errno,strerror(errno));
		return -1;
	}

	/* bind source address
	*/
	memset ((void*)&srcAddr, 0, sizeof(srcAddr));
	srcAddr.sin_family = AF_INET;
	srcAddr.sin_port = htons((u_short)srcPort);
	srcAddr.sin_addr.s_addr = inet_addr(srcIp);

	if (bind(fd, (struct sockaddr*)&srcAddr, sizeof(srcAddr)) < 0 ) {
		fprintf(stderr,"[socklib_connect_bind] bind fail; src:%s-%d, dst:%s-%d; errno=%d(%s)\n",
				srcIp, srcPort, dstIp, dstPort, errno, strerror(errno));
		close(fd);
		return -1;
	}

	/*
	*/
	if (socklib_setSockOption(fd) < 0) {
		close(fd);
		return -1;
	}

	/* connect to server
	*/
	memset ((void*)&dstAddr, 0, sizeof(dstAddr));
	dstAddr.sin_family = AF_INET;
	dstAddr.sin_port = htons((u_short)dstPort);
	dstAddr.sin_addr.s_addr = inet_addr(dstIp);

	signal (SIGALRM, socklib_dumHdlr);
	ualarm(100000,0); // --->> multi-thread인 경우 문제가 될수 있다.
	if (connect(fd, (struct sockaddr*)&dstAddr, sizeof(dstAddr)) < 0 ) {
		ualarm(0,0);
		close(fd);
#ifdef DEBUG
		//fprintf(stdout,"[socklib_connect_bind] connect fail; src:%s-%d, dst:%s-%d; errno=%d(%s)\n", srcIp, srcPort, dstIp, dstPort, errno, strerror(errno));
#endif
		return -1;
	}
	ualarm(0,0);

	if (socklib_setNonBlockMode(fd) < 0) {
		close(fd);
		return -1;
	}

	/* serverSockFdTbl에 destIpAddress, destPort, fd를 저장한다.
	*/
	fdTbl_index = socklib_addServerSockFdTbl (&dstAddr, dstPort, fd);
	if (fdTbl_index < 0) {
		close(fd);
		return -1;
	}

	/* add socket fd to readSockFdSet, exceptSockFdSet
	*/
	socklib_addSockFdSet(fd);

	return fd;

} /** End of socklib_connect_bind **/



/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
int socklib_connect_noALRM (char *ipAddr, int port)
{
	int		fd, fdTbl_index;
	struct sockaddr_in	dstAddr;

	if (!strcmp(ipAddr,""))
		return -1;

	/* create socket fd
	*/
	if ((fd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr,"[socklib_connect_noALRM] socket fail; errno=%d(%s)\n",errno,strerror(errno));
		return -1;
	}

	/*
	*/
	if (socklib_setSockOption(fd) < 0) {
		close(fd);
		return -1;
	}

	/* connect to server
	*/
	memset ((void*)&dstAddr, 0, sizeof(dstAddr));
	dstAddr.sin_family = AF_INET;
	dstAddr.sin_port = htons((u_short)port);
	dstAddr.sin_addr.s_addr = inet_addr(ipAddr);

	//signal (SIGALRM, socklib_dumHdlr);
	//ualarm (100000*socklib_connect_timer, 0); // --->> multi-thread인 경우 문제가 될수 있다.
	if (connect(fd, (struct sockaddr*)&dstAddr, sizeof(dstAddr)) < 0 ) {
#ifdef DEBUG
		//fprintf(stdout,"[socklib_connect_noALRM] connect fail[%s %d]; errno=%d(%s)\n",ipAddr,port,errno,strerror(errno));
#endif
		//ualarm(0,0);
		close(fd);
		return -1;
	}
	//ualarm(0,0);

	if (socklib_setNonBlockMode(fd) < 0) {
		close(fd);
		return -1;
	}

	/* serverSockFdTbl에 address, port, fd를 저장한다.
	*/
	fdTbl_index = socklib_addServerSockFdTbl (&dstAddr, port, fd);
	if (fdTbl_index < 0) {
		close(fd);
		return -1;
	}

	/* add socket fd to readSockFdSet, exceptSockFdSet
	*/
	socklib_addSockFdSet(fd);

	return fd;

} /** End of socklib_connect_noALRM **/



/*------------------------------------------------------------------------------
* 새로운 접속요구가 감지된 경우 호출되어 accept하고 새로이 생성된 client_fd를 return
* - allowClientAddrTbl에 등록된 address에서 접속된 경우만 허용한다.
------------------------------------------------------------------------------*/
int socklib_acceptNewConnection (int srvPort, int srvFd, int *fdTbl_index)
{
	int		fd, len;
	struct sockaddr_in	cliAddr;

	len = sizeof(cliAddr);
	memset ((void*)&cliAddr, 0, sizeof(cliAddr));

	if ((fd = accept(srvFd, (struct sockaddr*)&cliAddr, (unsigned int *)&len)) < 0) {
		fprintf(stderr,"[socklib_acceptNewConnection] accept fail; errno=%d(%s)\n",errno,strerror(errno));
		return -1;
	}

#ifdef RESTRICTED_CLIENT
	if (allowClientCnt > 0)
	{
		/* allowClientAddrTbl에 등록된 address에서 요청되었는지 확인한다.
		*/
		for (i=0; i < allowClientCnt; i++) {
			if (cliAddr.sin_addr.s_addr == allowClientAddrTbl[i])
				break;
		}
		if (i >= allowClientCnt) {
			char	tmpBuf[256];
			sprintf (tmpBuf,">>> isn't allow address(%s)\n", inet_ntoa(cliAddr.sin_addr));
			trclib_writeLogErr(__FILE__,__LINE__,tmpBuf);
#ifdef DEBUG
			fprintf(stdout,"[socklib_acceptNewConnection] isn't allow address(%s)\n", inet_ntoa(cliAddr.sin_addr));
#endif
			close(fd);
			return -1;
		}
	}
#endif /*RESTRICTED_CLIENT*/

	if (socklib_setSockOption(fd) < 0) {
		fprintf(stderr,"[socklib_acceptNewConnection] socklib_setSockOption fail\n");
		close(fd);
		return -1;
	}
	if (socklib_setNonBlockMode(fd) < 0) {
		close(fd);
		return -1;
	}

	/* clientSockFdTbl에 address, port, fd를 저장한다.
	*/
	*fdTbl_index = socklib_addClientSockFdTbl (&cliAddr, fd, srvPort);
	if (*fdTbl_index < 0) {
		close(fd);
		return -1;
	}

	/* add socket fd to readSockFdSet, exceptSockFdSet
	*/
	socklib_addSockFdSet(fd);

	return fd;


} /** End of socklib_acceptNewConnection **/



/*------------------------------------------------------------------------------
* 지정된 fd가 server로 접속된 fd이면 socklib_disconnectServerFd를 호출하고,
*	client가 접속한 fd이면 socklib_disconnectClientFd를 호출하여 접속을 해제한다.
------------------------------------------------------------------------------*/
int socklib_disconnectSockFd (int fd)
{
	int		i;

	for (i=0; i<SOCKLIB_MAX_SERVER_CNT; i++) {
		if (serverSockFdTbl[i].fd == fd) {
			return (socklib_disconnectServerFd(fd));
		}
	}

	for (i=0; i<SOCKLIB_MAX_CLIENT_CNT; i++) {
		if (clientSockFdTbl[i].fd == fd) {
			return (socklib_disconnectClientFd(fd));
		}
	}

#ifdef DEBUG
	fprintf(stdout,"[socklib_disconnectSockFd] not found fd \n");
#endif
	return -1;

} /** End of socklib_disconnectSockFd **/



/*------------------------------------------------------------------------------
* - 해당 fd를 close하고,
* - serverSockFdTbl에서 삭제하고,
* - read/except fd_set에서 삭제한다.
------------------------------------------------------------------------------*/
int socklib_disconnectServerFd (int fd)
{
	close(fd);

	/* serverSockFdTbl에서 fd에 대한 정보를 삭제한다.
	*/
	socklib_delServerSockFdTbl (fd);

	/* delete socket fd from readSockFdSet, exceptSockFdSet
	*/
	socklib_delSockFdSet(fd);

	return 1;

} /** End of socklib_disconnectServerFd **/



/*------------------------------------------------------------------------------
* - 해당 fd를 close하고,
* - clientSockFdTbl에서 삭제하고,
* - read/except fd_set에서 삭제한다.
------------------------------------------------------------------------------*/
int socklib_disconnectClientFd (int fd)
{
	close(fd);

	/* clientSockFdTbl에서 fd에 대한 정보를 삭제한다.
	*/
	socklib_delClientSockFdTbl (fd);

	/* delete socket fd from readSockFdSet, exceptSockFdSet
	*/
	socklib_delSockFdSet(fd);

	return 1;

} /** End of socklib_disconnectClientFd **/



/*------------------------------------------------------------------------------
* 등록된 fd들을 확인하여 event 발생여부를 확인한다.
* - event가 없으면 NO_EVENT를 return한다.
* - binding port에서 발생한 경우이면 새로운 client가 접속을 요구한 경우이므로
*	새로운 client_fd를 생성하여 accept한 후 actFd에 새로운 client_fd를 넘기고,
*	NEW_CONNECTION을 return한다.
* - client_fd 또는 server_fd에서 발생한 경우이면 메시지를 수신한 경우이거나 접속이
*	끊어진 경우이다.
*	-> 접속이 끊어진 경우 actFd에 disconnect된 fd를 넘기고, DISCONNECTED를 return한다.
*	-> 메시지를 수신한 경우 actFd에 메시지를 수신한 fd를 넘기고, buff에 수신된
*		메시지를 기록하고, SOCKLIB_MSG_RECEIVED를 return한다.
* - 기타 내부 error인 경우 SOCKLIB_INTERNAL_ERROR를 return한다.
------------------------------------------------------------------------------*/
int socklib_action (char *buff, int *actFd)
{
	int		port,fd,newFd, fdTbl_index;

	/* readSockFdSet/exceptSockFdSet를 확인하여 event 발생 여부를 확인한다.
	*/
	if (socklib_pollFdSet() <= 0)
		return SOCKLIB_NO_EVENT;

	/* binding port에서 event가 감지되었는지 확인한다.
	** - binding port 번호와 binding port의 socket fd가 return된다.
	*/
	if (socklib_lookupBindSockFdTbl (&port, &fd) > 0) {
		/* 새로운 client_fd를 생성한다.
		*/
		if ((newFd = socklib_acceptNewConnection (port, fd, &fdTbl_index)) < 0) {
			return SOCKLIB_INTERNAL_ERROR;
		}
		*actFd = newFd;
		return SOCKLIB_NEW_CONNECTION;
	}

	/* server로 연결된 fd에서 event가 감지되었는지 확인한다.
	*/
	if ((fd = socklib_lookupServerSockFdTbl (&fdTbl_index)) > 0) {
		/* 읽을 메시지가 있거나 disconnect된 경우인데, read를 시도해서 읽이지면
		**	메시지를 수신하고, read fail이면 disconnect된 것이다.
		*/
		*actFd = fd;
		if (socklib_readSockFd (buff, fd) < 0) {
			/* read fail이면 접속이 끊어진 것으로 판단하여 disconnect한다.
			*/
			socklib_disconnectServerFd (fd);
			return SOCKLIB_SERVER_DISCONNECTED;
		}
		return SOCKLIB_SERVER_MSG_RECEIVED;
	}

	/* client_fd에서 event가 감지되었는지 확인한다.
	*/
	if ((fd = socklib_lookupClientSockFdTbl (&fdTbl_index)) > 0) {
		/* 읽을 메시지가 있거나 disconnect된 경우인데, read를 시도해서 읽이지면
		**	메시지를 수신하고, read fail이면 disconnect된 것이다.
		*/
		*actFd = fd;
		if (socklib_readSockFd (buff, fd) < 0) {
			/* read fail이면 접속이 끊어진 것으로 판단하여 disconnect한다.
			*/
			socklib_disconnectClientFd (fd);
			return SOCKLIB_CLIENT_DISCONNECTED;
		}
		return SOCKLIB_CLIENT_MSG_RECEIVED;
	}

	return SOCKLIB_INTERNAL_ERROR;

} /** End of socklib_action **/



/*------------------------------------------------------------------------------
* event가 감지된 fd에서 메시지를 read하여 buff에 넣는다.
* - header의 크기만큼 먼저 읽고 header에 있는 length field를 보고 나머지 body를
*	읽는다.
------------------------------------------------------------------------------*/
int socklib_readSockFd (char *buff, int fd)
{
	int		i, len, rLen, bodyLen;
	char	*ptr;
#ifdef aaaaaa
	char	tmp[32];
#endif
	SockLibHeadType	*head;
	struct timeval	retryWaitTmr;

	retryWaitTmr.tv_sec  = 0;
	retryWaitTmr.tv_usec = SOCKLIB_RETRY_WAIT_TIMER;

	ptr = buff;

	/* header 부분만 먼저 읽는다.
	*/
	for (i=0, rLen=0;
		(i < SOCKLIB_RETRY_CNT) && (rLen < SOCKLIB_HEAD_LEN);
		i++)
	{
		len = (int)read(fd, ptr, SOCKLIB_HEAD_LEN-rLen);

		if (len == 0) {
#ifdef DEBUG
			fprintf(stdout,"[socklib_readSockFd] read fail(head,fd=%d); errno=%d(%s)\n",fd,errno,strerror(errno));
#endif
			return -1;
		} else if (len < 0) {
			if (errno==EAGAIN || errno==EINTR) {
#ifdef DEBUG
				fprintf(stdout,"[socklib_readSockFd] read would be blocked(head,fd=%d); errno=%d(%s)\n",fd,errno,strerror(errno));
#endif
				select (0,0,0,0,&retryWaitTmr);
				continue;
			}
#ifdef DEBUG
			fprintf(stdout,"[socklib_readSockFd] read fail(head,fd=%d); errno=%d(%s)\n",fd,errno,strerror(errno));
#endif
			return -1;
		}

		ptr += len;
		rLen += len;
	}
	if (i==SOCKLIB_RETRY_CNT && rLen < SOCKLIB_HEAD_LEN) {
		/* header length만큼 읽어내지 못한 경우 garbage데이터가 들어 있는 것으로
		**	생각할 수 있는데, 이 경우 -1을 return하여 disconnect하도록 한다.
		*/
#ifdef DEBUG
		fprintf(stdout,"[socklib_readSockFd] can't read data_head \n");
#endif
		return -1;
	}

	/* header에 있는 length field를 꺼낸다.
	*/
	head = (SockLibHeadType*)buff;
#ifdef aaaaaa
	memcpy (tmp, head->bodyLen, sizeof(head->bodyLen));
	tmp[sizeof(head->bodyLen)] = 0;
	bodyLen = atoi(tmp);
#else
	bodyLen = head->bodyLen = ntohl(head->bodyLen);
#endif

	/* bodyLen만큼 읽어낸다.
	*/
	for (i=0, rLen=0;
		(i<SOCKLIB_RETRY_CNT) && (rLen < bodyLen);
		i++)
	{
		len = (int)read(fd, ptr, (size_t)(bodyLen-rLen));

		if (len == 0) {
#ifdef DEBUG
			fprintf(stdout,"[socklib_readSockFd] read fail(body,fd=%d); errno=%d(%s)\n",fd,errno,strerror(errno));
#endif
			return -1;
		} else if (len < 0) {
			if (errno==EAGAIN || errno==EINTR) {
#ifdef DEBUG
				fprintf(stdout,"[socklib_readSockFd] read would be blocked(body,fd=%d); errno=%d(%s)\n",fd,errno,strerror(errno));
#endif
				select (0,0,0,0,&retryWaitTmr);
				continue;
			}
#ifdef DEBUG
			fprintf(stdout,"[socklib_readSockFd] read fail(body,fd=%d); errno=%d(%s)\n",fd,errno,strerror(errno));
#endif
			return -1;
		}

		ptr += len;
		rLen += len;
	}
	if (i==SOCKLIB_RETRY_CNT && rLen < bodyLen) {
		/* body length만큼 읽어내지 못한 경우 garbage데이터가 들어 있는 것으로
		**	생각할 수 있는데, 이 경우 -1을 return하여 disconnect하도록 한다.
		*/
#ifdef DEBUG
		fprintf(stdout,"[socklib_readSockFd] can't read data_body \n");
#endif
		return -1;
	}

	// 맨끝에 NULL을 채운다.
	*ptr = 0;

#ifdef DEBUG
	//fprintf(stdout,"[socklib_readSockFd] read len=%d\n", rLen+SOCKLIB_HEAD_LEN);
#endif
	return (int)(rLen+SOCKLIB_HEAD_LEN);

} /** End of socklib_readSockFd **/



/*------------------------------------------------------------------------------
* 지정된 fd에 데이터를 length만큼 write한다.
* - head.bodyLen를 network byte order로 바꾼다.
* - write하기전에 write 가능한지 확인한다.
*	- select로 확인하는데 select fail 시 fd를 강제로 close한다.
------------------------------------------------------------------------------*/
int socklib_sndMsg (int fd, char *buff, int buffLen)
{
	int		i,len,wLen,ret;
	struct timeval	retryWaitTmr;
	fd_set	wFdSet;
	char	*ptr;
	SockLibHeadType *head;

	/* head.bodyLen를 network byte order로 바꾼다.
	*/
	head = (SockLibHeadType*)buff;
	head->bodyLen = htonl(head->bodyLen);
	
	retryWaitTmr.tv_sec  = 0;
	retryWaitTmr.tv_usec = SOCKLIB_RETRY_WAIT_TIMER;

	FD_ZERO(&wFdSet);
	FD_SET (fd, &wFdSet);

	ptr = buff;

	for (i=0, wLen=0;
		(i<SOCKLIB_RETRY_CNT) && (wLen < buffLen);
		i++)
	{
		/* write할 수 있는지 확인한다.
		*/
		ret = select (fd+1, NULL, &wFdSet, NULL, &retryWaitTmr);
		if (ret == 0) {
#ifdef DEBUG
			fprintf(stdout,"[socklib_sndMsg] write would be blocked;\n");
#endif
			continue;
		} else if (ret < 0){
#ifdef DEBUG
			fprintf(stdout,"[socklib_sndMsg] select fail; errno=%d(%s)\n",errno,strerror(errno));
#endif
			/* serverSockFdTbl, clientSockFdTbl를 검색해 server로 접속된 fd인지,
			**	client가 접속한 fd인지 찾은 후 접속을 끊는다.
			*/
			socklib_disconnectSockFd (fd);
			return -1;
		}

		/* 해당 fd에 data를 write한다.
		*/
		len = (int)write(fd, ptr, (size_t)(buffLen-wLen));
		if (len < 0) {
			if (errno==EAGAIN || errno==EINTR) {
#ifdef DEBUG
				fprintf(stdout,"[socklib_sndMsg] write would be blocked; errno=%d(%s)\n",errno,strerror(errno));
#endif
				select (0,0,0,0,&retryWaitTmr);
				continue;
			}
			/* write fail이면 접속을 강제로 끊는다.
			*/
#ifdef DEBUG
			fprintf(stdout,"[socklib_sndMsg] write fail; errno=%d(%s)\n",errno,strerror(errno));
#endif
			socklib_disconnectSockFd (fd);
			return -1;
		}
		ptr += len;
		wLen += len;
	}
	//if (i==SOCKLIB_RETRY_CNT && wLen < buffLen)
	if (wLen < buffLen)
	{
		/* 지정된 length만큼 write하지 못한 경우 접속을 강제로 끊는다.
		*/
#ifdef DEBUG
		fprintf(stdout,"[socklib_sndMsg] can't write msg \n");
#endif
		socklib_disconnectSockFd (fd);
		return -1;
	}

#ifdef DEBUG
	//fprintf(stdout,"[socklib_sndMsg] write msg len = %d, fd=%d\n", wLen, fd);
#endif
	return wLen;

} /** End of socklib_sndMsg **/



/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
int socklib_broadcast (char *buff, int buffLen)
{
	socklib_broadcast2Servers (buff, buffLen);
	socklib_broadcast2Clients (buff, buffLen);
	return 1;
} /** End of socklib_broadcast **/



/*------------------------------------------------------------------------------
* server로 연결된 모든 fd에 메시지를 write한다.
------------------------------------------------------------------------------*/
int socklib_broadcast2Servers (char *buff, int buffLen)
{
	int	i;

	for (i=0; i<SOCKLIB_MAX_SERVER_CNT; i++) {
		if (serverSockFdTbl[i].fd) {
			socklib_sndMsg (serverSockFdTbl[i].fd, buff, buffLen);
		}
	}
	return 1;
} /** End of socklib_broadcast2Servers **/



/*------------------------------------------------------------------------------
* client가 접속한 모든 fd에 메시지를 write한다.
------------------------------------------------------------------------------*/
int socklib_broadcast2Clients (char *buff, int buffLen)
{
	int	i;

	for (i=0; i<SOCKLIB_MAX_CLIENT_CNT; i++) {
		if (clientSockFdTbl[i].fd) {
			socklib_sndMsg (clientSockFdTbl[i].fd, buff, buffLen);
		}
	}
	return 1;
} /** End of socklib_broadcast2Clients **/



/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
int socklib_setSockOption (int fd)
{
	int		sockOpt=1;
	int		reUseOpt=1;
	int		buffSize;
	struct linger	lin;
#ifdef TCPNODELAY
	struct protoent	*proto;
#endif

	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char*)&sockOpt, sizeof(sockOpt)) < 0) {
		fprintf(stderr,"[socklib_setSockOption] setsockopt(SO_KEEPALIVE) fail; errno=%d(%s)\n",errno,strerror(errno));
		return -1;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&reUseOpt, sizeof(reUseOpt)) < 0) {
		fprintf(stderr,"[socklib_setSockOption] setsockopt(SO_REUSEADDR) fail; errno=%d(%s)\n",errno,strerror(errno));
		return -1;
	}

#ifdef TRU64
	if (setsockopt(fd, SOL_SOCKET, SO_CLUA_IN_NOALIAS, (char*)&sockOpt, sizeof(sockOpt)) < 0) {
		fprintf(stderr,"[socklib_setSockOption] setsockopt(SO_CLUA_IN_NOALIAS) fail; errno=%d(%s)\n",errno,strerror(errno));
		return -1;
	}
#endif

#ifdef SOREUSEPORT
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPOER, (char*)&reUseOpt, sizeof(reUseOpt)) < 0) {
		fprintf(stderr,"[socklib_setSockOption] setsockopt(SO_REUSEPOER) fail; errno=%d(%s)\n",errno,strerror(errno));
		return -1;
	}
#endif /*SOREUSEPORT*/

	lin.l_onoff  = 1;
	lin.l_linger = 0;
	if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (char*)&lin, sizeof(lin)) < 0) {
		fprintf(stderr,"[socklib_setSockOption] setsockopt(SO_LINGER) fail; errno=%d(%s)\n",errno,strerror(errno));
		return -1;
	}

#ifdef TCPNODELAY
	if ((proto = getprotobyname("tcp")) == NULL) {
		fprintf(stderr,"[socklib_setSockOption] getprotobyname() fail\n");
		return -1;
	}
	if (setsockopt(fd, proto->p_proto, TCP_NODELAY, (char*)&sockOpt, sizeof(sockOpt)) < 0) {
		fprintf(stderr,"[socklib_setSockOption] setsockopt(TCP_NODELAY) fail; errno=%d(%s)\n",errno,strerror(errno));
		return -1;
	}
#endif /*TCPNODELAY*/

	/* set segment buffer size
	*/
	buffSize = SOCKLIB_SEG_BUF_SIZE;
	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void*)&buffSize, sizeof(buffSize)) < 0) {
		fprintf(stderr,"[socklib_setSockOption] setsockopt(SO_RCVBUFF) fail; errno=%d(%s)\n",errno,strerror(errno));
		return -1;
	}
	if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void*)&buffSize, sizeof(buffSize)) < 0) {
		fprintf(stderr,"[socklib_setSockOption] setsockopt(SO_SNDBUFF) fail; errno=%d(%s)\n",errno,strerror(errno));
		return -1;
	}

	return 1;

} /** End of socklib_setSockOption **/



/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
int socklib_setNonBlockMode (int fd)
{
	int		flag;

	/* set Non-Blocking Mode
	*/
	if ((flag = fcntl(fd, F_GETFL, 0)) < 0) {
		fprintf(stderr,"[socklib_setNonBlockMode] fcntl(F_GETFL) fail; errno=%d(%s)\n",errno,strerror(errno));
		return -1;
	}
	flag |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flag) < 0) {
		fprintf(stderr,"[socklib_setNonBlockMode] fcntl(F_SETFL) fail; errno=%d(%s)\n",errno,strerror(errno));
		return -1;
	}

	return 1;

} /** End of socklib_setNonBlockMode **/



/*------------------------------------------------------------------------------
* bindSockFdTbl에 binding port와 socket_fd를 저장한다.
* - event 감지시 bindSockFdTbl에 등록된 fd에서 event가 발생한 경우이면
*	client로부터의 새로운 connection 요구가 수신되었음을 알 수 있다.
------------------------------------------------------------------------------*/
int socklib_addBindSockFdTbl (int port, int fd)
{
	int		i;

	for (i=0; i<SOCKLIB_MAX_BIND_CNT; i++) {
		if (bindSockFdTbl[i].fd == 0) {
			bindSockFdTbl[i].fd   = fd;
			bindSockFdTbl[i].port = port;
			break;
		}
	}
	if (i == SOCKLIB_MAX_BIND_CNT) {
		fprintf(stderr,"[socklib_addBindSockFdTbl] bindSockFdTbl full \n");
		return -1;
	}

	return i;

} /** End of socklib_addBindSockFdTbl **/



/*------------------------------------------------------------------------------
* serverSockFdTbl에 server로 connect된 fd와 server의 address, port를 저장한다.
------------------------------------------------------------------------------*/
int socklib_addServerSockFdTbl (struct sockaddr_in *srvAddr, int port, int fd)
{
	int		i;

	for (i=0; i<SOCKLIB_MAX_SERVER_CNT; i++) {
		if (serverSockFdTbl[i].fd == 0) {
			serverSockFdTbl[i].fd   = fd;
			serverSockFdTbl[i].port = port;
			memcpy (&serverSockFdTbl[i].rmtAddr, srvAddr, sizeof(struct sockaddr_in));
			serverSockFdTbl[i].connectTime = time(0);
			break;
		}
	}
	if (i == SOCKLIB_MAX_SERVER_CNT) {
		fprintf(stderr,"[socklib_addServerSockFdTbl] serverSockFdTbl full \n");
		return -1;
	}

	return i;

} /** End of socklib_addServerSockFdTbl **/



/*------------------------------------------------------------------------------
* clientSockFdTbl에 client_fd와 client의 address를 저장한다.
* - 해당 client가 어떤 binding port로 접속했던 놈인지 구분할 수 있도록 binding port
*	번호를 함께 저장한다.
------------------------------------------------------------------------------*/
int socklib_addClientSockFdTbl (struct sockaddr_in *cliAddr, int fd, int srvPort)
{
	int		i;

	for (i=0; i<SOCKLIB_MAX_CLIENT_CNT; i++) {
		if (clientSockFdTbl[i].fd == 0) {
			clientSockFdTbl[i].fd   = fd;
			clientSockFdTbl[i].port = srvPort;
			memcpy (&clientSockFdTbl[i].rmtAddr, cliAddr, sizeof(struct sockaddr_in));
			clientSockFdTbl[i].connectTime = time(0);
			break;
		}
	}
	if (i == SOCKLIB_MAX_CLIENT_CNT) {
		fprintf(stderr,"[socklib_addClientSockFdTbl] clientSockFdTbl full \n");
		return -1;
	}

	return i;

} /** End of socklib_addClientSockFdTbl **/



/*------------------------------------------------------------------------------
* serverSockFdTbl에서 fd에 대한 정보를 삭제한다.
------------------------------------------------------------------------------*/
int socklib_delServerSockFdTbl (int fd)
{
	int		i;

	for (i=0; i<SOCKLIB_MAX_SERVER_CNT; i++) {
		if (serverSockFdTbl[i].fd == fd) {
			memset ((void*)&serverSockFdTbl[i], 0, sizeof(SockLibSockFdContext));
			break;
		}
	}
	if (i == SOCKLIB_MAX_SERVER_CNT) {
#ifdef DEBUG
		fprintf(stdout,"[socklib_delServerSockFdTbl] not found fd in serverSockFdTbl \n");
#endif
		return -1;
	}

	return i;

} /** End of socklib_delServerSockFdTbl **/



/*------------------------------------------------------------------------------
* clientSockFdTbl에서 fd에 대한 정보를 삭제한다.
------------------------------------------------------------------------------*/
int socklib_delClientSockFdTbl (int fd)
{
	int		i;

	for (i=0; i<SOCKLIB_MAX_CLIENT_CNT; i++) {
		if (clientSockFdTbl[i].fd == fd) {
			memset ((void*)&clientSockFdTbl[i], 0, sizeof(SockLibSockFdContext));
			break;
		}
	}
	if (i == SOCKLIB_MAX_CLIENT_CNT) {
#ifdef DEBUG
		fprintf(stdout,"[socklib_delClientSockFdTbl] not found fd in clientSockFdTbl \n");
#endif
		return -1;
	}

	return i;

} /** End of socklib_delClientSockFdTbl **/



/*------------------------------------------------------------------------------
* read/except fd_set에 추가한다.
* - 새로운 fd가 생성되면 readSockFdSet/exceptSockFdSet에 setting하고,
*	이를 확인하면 event를 감지할 수 있다.
------------------------------------------------------------------------------*/
int socklib_addSockFdSet (int fd)
{
	FD_SET (fd, &readSockFdSet);
	FD_SET (fd, &exceptSockFdSet);

	if (fd >= maxSockFdNum)
		maxSockFdNum = fd + 1;

	return 1;

} /** End of socklib_addSockFdSet **/



/*------------------------------------------------------------------------------
* read/except fd_set에서 해당 fd를 삭제한다.
------------------------------------------------------------------------------*/
int socklib_delSockFdSet (int fd)
{
	FD_CLR (fd, &readSockFdSet);
	FD_CLR (fd, &exceptSockFdSet);

	if ((fd+1) == maxSockFdNum)
		maxSockFdNum--;

	return 1;

} /** End of socklib_delSockFdSet **/



/*------------------------------------------------------------------------------
* readSockFdSet/exceptSockFdSet를 확인하여 event 발생 여부를 확인한다.
------------------------------------------------------------------------------*/
int socklib_pollFdSet (void)
{
	struct timeval	pollTmr;
	int		ret;

	memcpy (&rdSockFdSet, &readSockFdSet, sizeof(fd_set));
	memcpy (&exSockFdSet, &exceptSockFdSet, sizeof(fd_set));

	pollTmr.tv_sec  = 0;
	pollTmr.tv_usec = SOCKLIB_POLL_TIMER;

	ret = select(maxSockFdNum, &rdSockFdSet, NULL, &exSockFdSet, &pollTmr);

	return ret;

} /** End of socklib_pollFdSet **/



/*------------------------------------------------------------------------------
* event가 감지된 fd가 binding port인지 확인한다.
* - 어떤 binding port에서 감지되었는지 해당 port번호와 port의 socket fd가 return한다.
------------------------------------------------------------------------------*/
int socklib_lookupBindSockFdTbl (int *srvPort, int *srvFd)
{
	int		i;

	for (i=0; i<SOCKLIB_MAX_BIND_CNT; i++) {
		if (!bindSockFdTbl[i].fd)
			continue;
		if (FD_ISSET(bindSockFdTbl[i].fd, &rdSockFdSet)) {
			*srvPort = bindSockFdTbl[i].port;
			*srvFd   = bindSockFdTbl[i].fd;
			return 1;
		}
		if (FD_ISSET(bindSockFdTbl[i].fd, &exSockFdSet)) {
			*srvPort = bindSockFdTbl[i].port;
			*srvFd   = bindSockFdTbl[i].fd;
			return 1;
		}
	}
	return -1;

} /** End of socklib_lookupBindSockFdTbl **/



/*------------------------------------------------------------------------------
* event가 감지된 fd가 server로 연결된 fd인지 확인한다.
------------------------------------------------------------------------------*/
int socklib_lookupServerSockFdTbl (int *fdTbl_index)
{
	int		i;
	static int	last_check_index_srv = 0;

	for (i = (last_check_index_srv+1); i < SOCKLIB_MAX_SERVER_CNT; i++) {
		if (!serverSockFdTbl[i].fd)
			continue;
		if (FD_ISSET(serverSockFdTbl[i].fd, &rdSockFdSet)) {
			last_check_index_srv = *fdTbl_index = i;
			return serverSockFdTbl[i].fd;
		}
		if (FD_ISSET(serverSockFdTbl[i].fd, &exSockFdSet)) {
			last_check_index_srv = *fdTbl_index = i;
			return serverSockFdTbl[i].fd;
		}
	}
	for (i = 0; i <= last_check_index_srv; i++) {
		if (!serverSockFdTbl[i].fd)
			continue;
		if (FD_ISSET(serverSockFdTbl[i].fd, &rdSockFdSet)) {
			last_check_index_srv = *fdTbl_index = i;
			return serverSockFdTbl[i].fd;
		}
		if (FD_ISSET(serverSockFdTbl[i].fd, &exSockFdSet)) {
			last_check_index_srv = *fdTbl_index = i;
			return serverSockFdTbl[i].fd;
		}
	}
	return -1;

} /** End of socklib_lookupServerSockFdTbl **/



/*------------------------------------------------------------------------------
* event가 감지된 fd가 client_fd인지 확인한다.
------------------------------------------------------------------------------*/
int socklib_lookupClientSockFdTbl (int *fdTbl_index)
{
	int		i;
	static int	last_check_index_cli = 0;

	for (i = (last_check_index_cli+1); i < SOCKLIB_MAX_CLIENT_CNT; i++) {
		if (!clientSockFdTbl[i].fd)
			continue;
		if (FD_ISSET(clientSockFdTbl[i].fd, &rdSockFdSet)) {
			last_check_index_cli = *fdTbl_index = i;
			return clientSockFdTbl[i].fd;
		}
		if (FD_ISSET(clientSockFdTbl[i].fd, &exSockFdSet)) {
			last_check_index_cli = *fdTbl_index = i;
			return clientSockFdTbl[i].fd;
		}
	}
	for (i = 0; i <= last_check_index_cli; i++) {
		if (!clientSockFdTbl[i].fd)
			continue;
		if (FD_ISSET(clientSockFdTbl[i].fd, &rdSockFdSet)) {
			last_check_index_cli = *fdTbl_index = i;
			return clientSockFdTbl[i].fd;
		}
		if (FD_ISSET(clientSockFdTbl[i].fd, &exSockFdSet)) {
			last_check_index_cli = *fdTbl_index = i;
			return clientSockFdTbl[i].fd;
		}
	}
	return -1;

} /** End of socklib_lookupClientSockFdTbl **/



/*------------------------------------------------------------------------------
* 등록된 fd들을 확인하여 event 발생여부를 확인한다.
* - event가 없으면 NO_EVENT를 return한다.
* - binding port에서 발생한 경우이면 새로운 client가 접속을 요구한 경우이므로
*	새로운 client_fd를 생성하여 accept한 후 actFd에 새로운 client_fd를 넘기고,
*	NEW_CONNECTION을 return한다.
* - client_fd 또는 server_fd에서 발생한 경우이면 메시지를 수신한 경우이거나 접속이
*	끊어진 경우이다.
*	-> 접속이 끊어진 경우 actFd에 disconnect된 fd를 넘기고, DISCONNECTED를 return한다.
*	-> 메시지를 수신한 경우 actFd에 메시지를 수신한 fd를 넘기고, rcvBuf에 수신된
*      메시지를 기록하고, SOCKLIB_MSG_RECEIVED를 return한다.
* - 기타 내부 error인 경우 SOCKLIB_INTERNAL_ERROR를 return한다.
------------------------------------------------------------------------------*/
int socklib_action_2 (int *actFd, int *fdTbl_index)
{
	int		port,fd,newFd;

	/* readSockFdSet/exceptSockFdSet를 확인하여 event 발생 여부를 확인한다.
	*/
	if (socklib_pollFdSet() <= 0)
		return SOCKLIB_NO_EVENT;

	/* binding port에서 event가 감지되었는지 확인한다.
	** - binding port 번호와 binding port의 socket fd가 return된다.
	*/
	if (socklib_lookupBindSockFdTbl (&port, &fd) > 0) {
		/* 새로운 client_fd를 생성한다.
		*/
		if ((newFd = socklib_acceptNewConnection (port, fd, fdTbl_index)) < 0) {
			return SOCKLIB_INTERNAL_ERROR;
		}
		*actFd = newFd;
		return SOCKLIB_NEW_CONNECTION;
	}

	/* server로 연결된 fd에서 event가 감지되었는지 확인한다.
	*/
	if ((fd = socklib_lookupServerSockFdTbl (fdTbl_index)) > 0) {
		/* 읽을 메시지가 있거나 disconnect된 경우인데, read를 시도해서 읽이지면
		**	메시지를 수신하고, read fail이면 disconnect된 것이다.
		*/
		*actFd  = fd;
		if (socklib_readSockFd_2 (&serverSockFdTbl[*fdTbl_index]) < 0) {
			/* read fail이면 접속이 끊어진 것으로 판단하여 disconnect한다.
			*/
			socklib_disconnectServerFd (fd);
			return SOCKLIB_SERVER_DISCONNECTED;
		}
		return SOCKLIB_SERVER_MSG_RECEIVED;
	}

	/* client_fd에서 event가 감지되었는지 확인한다.
	*/
	if ((fd = socklib_lookupClientSockFdTbl (fdTbl_index)) > 0) {
		/* 읽을 메시지가 있거나 disconnect된 경우인데, read를 시도해서 읽이지면
		**	메시지를 수신하고, read fail이면 disconnect된 것이다.
		*/
		*actFd  = fd;
		if (socklib_readSockFd_2 (&clientSockFdTbl[*fdTbl_index]) < 0) {
			/* read fail이면 접속이 끊어진 것으로 판단하여 disconnect한다.
			*/
			socklib_disconnectClientFd (fd);
			return SOCKLIB_CLIENT_DISCONNECTED;
		}
		return SOCKLIB_CLIENT_MSG_RECEIVED;
	}

	return SOCKLIB_INTERNAL_ERROR;

} /** End of socklib_action_2 **/



/*------------------------------------------------------------------------------
* event가 감지된 fd에서 메시지를 read하여 rcvBuf에 넣는다.
* - 무조건 read해서 rcvBuf에 쌓는다.
* - rcvBuf에 이전에 읽혀졌지만 Application에서 처리하지 않고 남아있는 메시지가 rcvBuf에
*   있을 수 있다. 이번에 읽혀진 데이터를 rcvBuf에 이어 붙인다.
------------------------------------------------------------------------------*/
int socklib_readSockFd_2 (SockLibSockFdContext *pFdTbl)
{
	int		len, offset, rcvBuf_freeLen;

	// rcvBuf에 이전에 받고 아직 처리하지 않는 데이터가 남아 있을 수 있고,
	//   남아 있는 데이터의 길이가 rcvMsgLen이다.
	// 이번에 읽은 데이터를 그 뒤에 붙여가 하므로 그 offset을 먼저 꺼낸다.
	// 이번에 최대로 읽을 수 있는 max를 결정할때도 현재 들어있는 데이터의 크기에 따라 다르다.
	//
	offset = pFdTbl->rcvMsgLen;
	rcvBuf_freeLen = SOCKLIB_RCV_BUF_SIZE - pFdTbl->rcvMsgLen - 1;

	len = (int)read (pFdTbl->fd, &pFdTbl->rcvBuf[offset], (size_t)rcvBuf_freeLen);

	if (len == 0) {
#ifdef DEBUG
		fprintf(stdout,"[socklib_readSockFd_2] read fail(fd=%d); errno=%d(%s)\n", pFdTbl->fd, errno, strerror(errno));
#endif
		return -1;
	}
	else if (len < 0) {
		if (errno != EAGAIN && errno != EINTR) {
#ifdef DEBUG
			fprintf(stdout,"[socklib_readSockFd_2] read fail(fd=%d); errno=%d(%s)\n", pFdTbl->fd, errno, strerror(errno));
#endif
			return -1;
		}
		// 밑에서 rcvMsgLen를 조절하므로 이번에 읽힌 데이터길이가 0이라고 변경한다.
		len = 0;
	}

	// 이번에 읽어낸 데이터 길이만큼 rcvMsgLen를 update한다.
	// 맨끝에 NULL을 채운다.
	//
	offset = pFdTbl->rcvMsgLen += len;
	pFdTbl->rcvBuf[offset] = 0;

	// return값은 rcvBuf에 들어있는 데이터의 길이를 return한다.
	// 이전에 남아있는 데어터가 있고 이번에 읽혀지지 않았어도 0보다 큰 값이 return되도록 하기 위해
	//
#ifdef DEBUG
	//fprintf(stdout,"[socklib_readSockFd_2] read len=%d, ret=%d\n", len, offset);
#endif
	return (offset);

} /** End of socklib_readSockFd_2 **/



/*------------------------------------------------------------------------------
* 지정된 fd에 데이터를 length만큼 write한다.
* - write하기전에 write 가능한지 확인한다.
*	- select로 확인하는데 select fail 시 fd를 강제로 close한다.
------------------------------------------------------------------------------*/
int socklib_sndMsg_2 (int fd, char *buff, int buffLen)
{
	int		i,len,wLen,ret;
	struct timeval	retryWaitTmr;
	fd_set	wFdSet;
	char	*ptr;

	retryWaitTmr.tv_sec  = 0;
	retryWaitTmr.tv_usec = SOCKLIB_RETRY_WAIT_TIMER;

	FD_ZERO(&wFdSet);
	FD_SET (fd, &wFdSet);

	ptr = buff;

	for (i=0, wLen=0;
		(i<SOCKLIB_RETRY_CNT) && (wLen < buffLen);
		i++)
	{
		/* write할 수 있는지 확인한다.
		*/
		ret = select (fd+1, NULL, &wFdSet, NULL, &retryWaitTmr);
		if (ret == 0) {
#ifdef DEBUG
			fprintf(stdout,"[socklib_sndMsg_2] write would be blocked;\n");
#endif
			continue;
		} else if (ret < 0){
#ifdef DEBUG
			fprintf(stdout,"[socklib_sndMsg_2] select fail; errno=%d(%s)\n",errno,strerror(errno));
#endif
			/* serverSockFdTbl, clientSockFdTbl를 검색해 server로 접속된 fd인지,
			**	client가 접속한 fd인지 찾은 후 접속을 끊는다.
			*/
			socklib_disconnectSockFd (fd);
			return -1;
		}

		/* 해당 fd에 data를 write한다.
		*/
		len = (int)write(fd, ptr, (size_t)(buffLen-wLen));
		if (len < 0) {
			if (errno==EAGAIN || errno==EINTR) {
#ifdef DEBUG
				fprintf(stdout,"[socklib_sndMsg_2] write would be blocked; errno=%d(%s)\n",errno,strerror(errno));
#endif
				select (0,0,0,0,&retryWaitTmr);
				continue;
			}
			/* write fail이면 접속을 강제로 끊는다.
			*/
#ifdef DEBUG
			fprintf(stdout,"[socklib_sndMsg_2] write fail; errno=%d(%s)\n",errno,strerror(errno));
#endif
			socklib_disconnectSockFd (fd);
			return -1;
		}
		ptr += len;
		wLen += len;
	}
	//if (i==SOCKLIB_RETRY_CNT && wLen < buffLen)
	if (wLen < buffLen)
	{
		/* 지정된 length만큼 write하지 못한 경우 접속을 강제로 끊는다.
		*/
#ifdef DEBUG
		fprintf(stdout,"[socklib_sndMsg_2] can't write msg \n");
#endif
		socklib_disconnectSockFd (fd);
		return -1;
	}

#ifdef DEBUG
	//fprintf(stdout,"[socklib_sndMsg_2] write msg len = %d, fd=%d\n", wLen, fd);
#endif
	return wLen;

} /** End of socklib_sndMsg_2 **/



/*------------------------------------------------------------------------------
* 지정된 ipAddr,port로 socket fd를 생성하여 접속한다.
* - 생성된 fd를 read/except fd_set에 추가한다.
* - 생성된 fd가 return된다.
------------------------------------------------------------------------------*/
int socklib_connect_2 (char *ipAddr, int port, int *fdTbl_index)
{
	int		fd;
	struct sockaddr_in	dstAddr;

	if (!strcmp(ipAddr,""))
		return -1;

	/* create socket fd
	*/
	if ((fd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr,"[socklib_connect_2] socket fail; errno=%d(%s)\n",errno,strerror(errno));
		return -1;
	}

	/*
	*/
	if (socklib_setSockOption(fd) < 0) {
		close(fd);
		return -1;
	}

	/* connect to server
	*/
	memset ((void*)&dstAddr, 0, sizeof(dstAddr));
	dstAddr.sin_family = AF_INET;
	dstAddr.sin_port = htons((u_short)port);
	dstAddr.sin_addr.s_addr = inet_addr(ipAddr);

	signal (SIGALRM, socklib_dumHdlr);
	//ualarm ((useconds_t)(100000*socklib_connect_timer), 0); // --->> multi-thread인 경우 문제가 될수 있다.
	ualarm (100000*socklib_connect_timer, 0); // --->> multi-thread인 경우 문제가 될수 있다.
	if (connect(fd, (struct sockaddr*)&dstAddr, sizeof(dstAddr)) < 0 ) {
#ifdef DEBUG
		//fprintf(stdout,"[socklib_connect_2] connect fail[%s %d]; errno=%d(%s)\n",ipAddr,port,errno,strerror(errno));
#endif
		ualarm(0,0);
		close(fd);
		return -1;
	}
	ualarm(0,0);

	if (socklib_setNonBlockMode(fd) < 0) {
		close(fd);
		return -1;
	}

	/* serverSockFdTbl에 address, port, fd를 저장한다.
	*/
	*fdTbl_index = socklib_addServerSockFdTbl (&dstAddr, port, fd);
	if (*fdTbl_index < 0) {
		close(fd);
		return -1;
	}

	/* add socket fd to readSockFdSet, exceptSockFdSet
	*/
	socklib_addSockFdSet(fd);

	return fd;

} /** End of socklib_connect_2 **/



/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
int socklib_connect_noALRM_2 (char *ipAddr, int port, int *fdTbl_index)
{
	int		fd;
	struct sockaddr_in	dstAddr;

	if (!strcmp(ipAddr,""))
		return -1;

	/* create socket fd
	*/
	if ((fd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr,"[socklib_connect_noALRM_2] socket fail; errno=%d(%s)\n",errno,strerror(errno));
		return -1;
	}

	/*
	*/
	if (socklib_setSockOption(fd) < 0) {
		close(fd);
		return -1;
	}

	/* connect to server
	*/
	memset ((void*)&dstAddr, 0, sizeof(dstAddr));
	dstAddr.sin_family = AF_INET;
	dstAddr.sin_port = htons((u_short)port);
	dstAddr.sin_addr.s_addr = inet_addr(ipAddr);

	//signal (SIGALRM, socklib_dumHdlr);
	//ualarm (100000*socklib_connect_timer, 0); // --->> multi-thread인 경우 문제가 될수 있다.
	if (connect(fd, (struct sockaddr*)&dstAddr, sizeof(dstAddr)) < 0 ) {
#ifdef DEBUG
		//fprintf(stdout,"[socklib_connect_noALRM_2] connect fail[%s %d]; errno=%d(%s)\n",ipAddr,port,errno,strerror(errno));
#endif
		//ualarm(0,0);
		close(fd);
		return -1;
	}
	//ualarm(0,0);

	if (socklib_setNonBlockMode(fd) < 0) {
		close(fd);
		return -1;
	}

	/* serverSockFdTbl에 address, port, fd를 저장한다.
	*/
	*fdTbl_index = socklib_addServerSockFdTbl (&dstAddr, port, fd);
	if (*fdTbl_index < 0) {
		close(fd);
		return -1;
	}

	/* add socket fd to readSockFdSet, exceptSockFdSet
	*/
	socklib_addSockFdSet(fd);

	return fd;

} /** End of socklib_connect_noALRM_2 **/






/*------------------------------------------------------------------------------
* 지정된 srcIpAddr,port를 갖고, dstIp,port로 socket fd를 생성하여 접속한다.
* - 생성된 fd를 read/except fd_set에 추가한다.
* - 생성된 fd가 return된다.
------------------------------------------------------------------------------*/
int socklib_connect_bind_2 (char *dstIp, int dstPort, char *srcIp, int srcPort, int *fdTbl_index)
{
	int		fd;
	struct sockaddr_in	dstAddr;
	struct sockaddr_in	srcAddr;

	if (!strcmp(dstIp,""))
		return -1;

	/* create socket fd
	*/
	if ((fd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr,"[socklib_connect_bind_2] socket fail; errno=%d(%s)\n",errno,strerror(errno));
		return -1;
	}

	/* bind source address
	*/
	memset ((void*)&srcAddr, 0, sizeof(srcAddr));
	srcAddr.sin_family = AF_INET;
	srcAddr.sin_port = htons((u_short)srcPort);
	srcAddr.sin_addr.s_addr = inet_addr(srcIp);

	if (bind(fd, (struct sockaddr*)&srcAddr, sizeof(srcAddr)) < 0 ) {
		fprintf(stderr,"[socklib_connect_bind_2] bind fail; src:%s-%d, dst:%s-%d; errno=%d(%s)\n",
				srcIp, srcPort, dstIp, dstPort, errno, strerror(errno));
		close(fd);
		return -1;
	}

	/*
	*/
	if (socklib_setSockOption(fd) < 0) {
		close(fd);
		return -1;
	}

	/* connect to server
	*/
	memset ((void*)&dstAddr, 0, sizeof(dstAddr));
	dstAddr.sin_family = AF_INET;
	dstAddr.sin_port = htons((u_short)dstPort);
	dstAddr.sin_addr.s_addr = inet_addr(dstIp);

	signal (SIGALRM, socklib_dumHdlr);
	ualarm(100000,0); // --->> multi-thread인 경우 문제가 될수 있다.
	if (connect(fd, (struct sockaddr*)&dstAddr, sizeof(dstAddr)) < 0 ) {
		ualarm(0,0);
		close(fd);
#ifdef DEBUG
		//fprintf(stdout,"[socklib_connect_bind_2] connect fail; src:%s-%d, dst:%s-%d; errno=%d(%s)\n", srcIp, srcPort, dstIp, dstPort, errno, strerror(errno));
#endif
		return -1;
	}
	ualarm(0,0);

	if (socklib_setNonBlockMode(fd) < 0) {
		close(fd);
		return -1;
	}

	/* serverSockFdTbl에 destIpAddress, destPort, fd를 저장한다.
	*/
	*fdTbl_index = socklib_addServerSockFdTbl (&dstAddr, dstPort, fd);
	if (*fdTbl_index < 0) {
		close(fd);
		return -1;
	}

	/* add socket fd to readSockFdSet, exceptSockFdSet
	*/
	socklib_addSockFdSet(fd);

	return fd;

} /** End of socklib_connect_bind_2 **/



