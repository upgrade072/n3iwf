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
* socklib���� ����� global variable�� �ʱ�ȭ �Ѵ�.
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
* ������ file�� �о� allowClientAddrTbl�� setting�Ѵ�.
* - client�κ��� connect �䱸�� ���� �Ǿ����� ���⿡ ��ϵ� address�� ����Ѵ�.
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
* ������ port�� socket fd�� �����ϰ�, bind/listen�Ѵ�.
* - ������ fd�� read/except fd_set�� �߰��Ѵ�.
* - � binding port�� connect �䱸�� �����Ǿ����� Ȯ���ϱ� ���� ������ fd��
*	bindSockFdTbl�� �߰��Ѵ�.
*	-> ��, �������� bindin port�� ����/������ �� �ִ�.
* - binding�� INADDR_ANY�� �����ν�, �ý��ۿ� ������ ��� ip_address�� ����
*	binding �ȴ�.
* - ������ fd�� return�ȴ�.
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

	/* bindSockFdTbl�� port�� fd�� �����Ѵ�.
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
* ������ ipAddr,port�� socket fd�� �����Ͽ� �����Ѵ�.
* - ������ fd�� read/except fd_set�� �߰��Ѵ�.
* - ������ fd�� return�ȴ�.
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
	//ualarm ((useconds_t)(100000*socklib_connect_timer), 0); // --->> multi-thread�� ��� ������ �ɼ� �ִ�.
	ualarm (100000*socklib_connect_timer, 0); // --->> multi-thread�� ��� ������ �ɼ� �ִ�.
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

	/* serverSockFdTbl�� address, port, fd�� �����Ѵ�.
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
* ������ srcIpAddr,port�� ����, dstIp,port�� socket fd�� �����Ͽ� �����Ѵ�.
* - ������ fd�� read/except fd_set�� �߰��Ѵ�.
* - ������ fd�� return�ȴ�.
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
	ualarm(100000,0); // --->> multi-thread�� ��� ������ �ɼ� �ִ�.
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

	/* serverSockFdTbl�� destIpAddress, destPort, fd�� �����Ѵ�.
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
	//ualarm (100000*socklib_connect_timer, 0); // --->> multi-thread�� ��� ������ �ɼ� �ִ�.
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

	/* serverSockFdTbl�� address, port, fd�� �����Ѵ�.
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
* ���ο� ���ӿ䱸�� ������ ��� ȣ��Ǿ� accept�ϰ� ������ ������ client_fd�� return
* - allowClientAddrTbl�� ��ϵ� address���� ���ӵ� ��츸 ����Ѵ�.
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
		/* allowClientAddrTbl�� ��ϵ� address���� ��û�Ǿ����� Ȯ���Ѵ�.
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

	/* clientSockFdTbl�� address, port, fd�� �����Ѵ�.
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
* ������ fd�� server�� ���ӵ� fd�̸� socklib_disconnectServerFd�� ȣ���ϰ�,
*	client�� ������ fd�̸� socklib_disconnectClientFd�� ȣ���Ͽ� ������ �����Ѵ�.
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
* - �ش� fd�� close�ϰ�,
* - serverSockFdTbl���� �����ϰ�,
* - read/except fd_set���� �����Ѵ�.
------------------------------------------------------------------------------*/
int socklib_disconnectServerFd (int fd)
{
	close(fd);

	/* serverSockFdTbl���� fd�� ���� ������ �����Ѵ�.
	*/
	socklib_delServerSockFdTbl (fd);

	/* delete socket fd from readSockFdSet, exceptSockFdSet
	*/
	socklib_delSockFdSet(fd);

	return 1;

} /** End of socklib_disconnectServerFd **/



/*------------------------------------------------------------------------------
* - �ش� fd�� close�ϰ�,
* - clientSockFdTbl���� �����ϰ�,
* - read/except fd_set���� �����Ѵ�.
------------------------------------------------------------------------------*/
int socklib_disconnectClientFd (int fd)
{
	close(fd);

	/* clientSockFdTbl���� fd�� ���� ������ �����Ѵ�.
	*/
	socklib_delClientSockFdTbl (fd);

	/* delete socket fd from readSockFdSet, exceptSockFdSet
	*/
	socklib_delSockFdSet(fd);

	return 1;

} /** End of socklib_disconnectClientFd **/



/*------------------------------------------------------------------------------
* ��ϵ� fd���� Ȯ���Ͽ� event �߻����θ� Ȯ���Ѵ�.
* - event�� ������ NO_EVENT�� return�Ѵ�.
* - binding port���� �߻��� ����̸� ���ο� client�� ������ �䱸�� ����̹Ƿ�
*	���ο� client_fd�� �����Ͽ� accept�� �� actFd�� ���ο� client_fd�� �ѱ��,
*	NEW_CONNECTION�� return�Ѵ�.
* - client_fd �Ǵ� server_fd���� �߻��� ����̸� �޽����� ������ ����̰ų� ������
*	������ ����̴�.
*	-> ������ ������ ��� actFd�� disconnect�� fd�� �ѱ��, DISCONNECTED�� return�Ѵ�.
*	-> �޽����� ������ ��� actFd�� �޽����� ������ fd�� �ѱ��, buff�� ���ŵ�
*		�޽����� ����ϰ�, SOCKLIB_MSG_RECEIVED�� return�Ѵ�.
* - ��Ÿ ���� error�� ��� SOCKLIB_INTERNAL_ERROR�� return�Ѵ�.
------------------------------------------------------------------------------*/
int socklib_action (char *buff, int *actFd)
{
	int		port,fd,newFd, fdTbl_index;

	/* readSockFdSet/exceptSockFdSet�� Ȯ���Ͽ� event �߻� ���θ� Ȯ���Ѵ�.
	*/
	if (socklib_pollFdSet() <= 0)
		return SOCKLIB_NO_EVENT;

	/* binding port���� event�� �����Ǿ����� Ȯ���Ѵ�.
	** - binding port ��ȣ�� binding port�� socket fd�� return�ȴ�.
	*/
	if (socklib_lookupBindSockFdTbl (&port, &fd) > 0) {
		/* ���ο� client_fd�� �����Ѵ�.
		*/
		if ((newFd = socklib_acceptNewConnection (port, fd, &fdTbl_index)) < 0) {
			return SOCKLIB_INTERNAL_ERROR;
		}
		*actFd = newFd;
		return SOCKLIB_NEW_CONNECTION;
	}

	/* server�� ����� fd���� event�� �����Ǿ����� Ȯ���Ѵ�.
	*/
	if ((fd = socklib_lookupServerSockFdTbl (&fdTbl_index)) > 0) {
		/* ���� �޽����� �ְų� disconnect�� ����ε�, read�� �õ��ؼ� ��������
		**	�޽����� �����ϰ�, read fail�̸� disconnect�� ���̴�.
		*/
		*actFd = fd;
		if (socklib_readSockFd (buff, fd) < 0) {
			/* read fail�̸� ������ ������ ������ �Ǵ��Ͽ� disconnect�Ѵ�.
			*/
			socklib_disconnectServerFd (fd);
			return SOCKLIB_SERVER_DISCONNECTED;
		}
		return SOCKLIB_SERVER_MSG_RECEIVED;
	}

	/* client_fd���� event�� �����Ǿ����� Ȯ���Ѵ�.
	*/
	if ((fd = socklib_lookupClientSockFdTbl (&fdTbl_index)) > 0) {
		/* ���� �޽����� �ְų� disconnect�� ����ε�, read�� �õ��ؼ� ��������
		**	�޽����� �����ϰ�, read fail�̸� disconnect�� ���̴�.
		*/
		*actFd = fd;
		if (socklib_readSockFd (buff, fd) < 0) {
			/* read fail�̸� ������ ������ ������ �Ǵ��Ͽ� disconnect�Ѵ�.
			*/
			socklib_disconnectClientFd (fd);
			return SOCKLIB_CLIENT_DISCONNECTED;
		}
		return SOCKLIB_CLIENT_MSG_RECEIVED;
	}

	return SOCKLIB_INTERNAL_ERROR;

} /** End of socklib_action **/



/*------------------------------------------------------------------------------
* event�� ������ fd���� �޽����� read�Ͽ� buff�� �ִ´�.
* - header�� ũ�⸸ŭ ���� �а� header�� �ִ� length field�� ���� ������ body��
*	�д´�.
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

	/* header �κи� ���� �д´�.
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
		/* header length��ŭ �о�� ���� ��� garbage�����Ͱ� ��� �ִ� ������
		**	������ �� �ִµ�, �� ��� -1�� return�Ͽ� disconnect�ϵ��� �Ѵ�.
		*/
#ifdef DEBUG
		fprintf(stdout,"[socklib_readSockFd] can't read data_head \n");
#endif
		return -1;
	}

	/* header�� �ִ� length field�� ������.
	*/
	head = (SockLibHeadType*)buff;
#ifdef aaaaaa
	memcpy (tmp, head->bodyLen, sizeof(head->bodyLen));
	tmp[sizeof(head->bodyLen)] = 0;
	bodyLen = atoi(tmp);
#else
	bodyLen = head->bodyLen = ntohl(head->bodyLen);
#endif

	/* bodyLen��ŭ �о��.
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
		/* body length��ŭ �о�� ���� ��� garbage�����Ͱ� ��� �ִ� ������
		**	������ �� �ִµ�, �� ��� -1�� return�Ͽ� disconnect�ϵ��� �Ѵ�.
		*/
#ifdef DEBUG
		fprintf(stdout,"[socklib_readSockFd] can't read data_body \n");
#endif
		return -1;
	}

	// �ǳ��� NULL�� ä���.
	*ptr = 0;

#ifdef DEBUG
	//fprintf(stdout,"[socklib_readSockFd] read len=%d\n", rLen+SOCKLIB_HEAD_LEN);
#endif
	return (int)(rLen+SOCKLIB_HEAD_LEN);

} /** End of socklib_readSockFd **/



/*------------------------------------------------------------------------------
* ������ fd�� �����͸� length��ŭ write�Ѵ�.
* - head.bodyLen�� network byte order�� �ٲ۴�.
* - write�ϱ����� write �������� Ȯ���Ѵ�.
*	- select�� Ȯ���ϴµ� select fail �� fd�� ������ close�Ѵ�.
------------------------------------------------------------------------------*/
int socklib_sndMsg (int fd, char *buff, int buffLen)
{
	int		i,len,wLen,ret;
	struct timeval	retryWaitTmr;
	fd_set	wFdSet;
	char	*ptr;
	SockLibHeadType *head;

	/* head.bodyLen�� network byte order�� �ٲ۴�.
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
		/* write�� �� �ִ��� Ȯ���Ѵ�.
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
			/* serverSockFdTbl, clientSockFdTbl�� �˻��� server�� ���ӵ� fd����,
			**	client�� ������ fd���� ã�� �� ������ ���´�.
			*/
			socklib_disconnectSockFd (fd);
			return -1;
		}

		/* �ش� fd�� data�� write�Ѵ�.
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
			/* write fail�̸� ������ ������ ���´�.
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
		/* ������ length��ŭ write���� ���� ��� ������ ������ ���´�.
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
* server�� ����� ��� fd�� �޽����� write�Ѵ�.
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
* client�� ������ ��� fd�� �޽����� write�Ѵ�.
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
* bindSockFdTbl�� binding port�� socket_fd�� �����Ѵ�.
* - event ������ bindSockFdTbl�� ��ϵ� fd���� event�� �߻��� ����̸�
*	client�κ����� ���ο� connection �䱸�� ���ŵǾ����� �� �� �ִ�.
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
* serverSockFdTbl�� server�� connect�� fd�� server�� address, port�� �����Ѵ�.
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
* clientSockFdTbl�� client_fd�� client�� address�� �����Ѵ�.
* - �ش� client�� � binding port�� �����ߴ� ������ ������ �� �ֵ��� binding port
*	��ȣ�� �Բ� �����Ѵ�.
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
* serverSockFdTbl���� fd�� ���� ������ �����Ѵ�.
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
* clientSockFdTbl���� fd�� ���� ������ �����Ѵ�.
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
* read/except fd_set�� �߰��Ѵ�.
* - ���ο� fd�� �����Ǹ� readSockFdSet/exceptSockFdSet�� setting�ϰ�,
*	�̸� Ȯ���ϸ� event�� ������ �� �ִ�.
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
* read/except fd_set���� �ش� fd�� �����Ѵ�.
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
* readSockFdSet/exceptSockFdSet�� Ȯ���Ͽ� event �߻� ���θ� Ȯ���Ѵ�.
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
* event�� ������ fd�� binding port���� Ȯ���Ѵ�.
* - � binding port���� �����Ǿ����� �ش� port��ȣ�� port�� socket fd�� return�Ѵ�.
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
* event�� ������ fd�� server�� ����� fd���� Ȯ���Ѵ�.
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
* event�� ������ fd�� client_fd���� Ȯ���Ѵ�.
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
* ��ϵ� fd���� Ȯ���Ͽ� event �߻����θ� Ȯ���Ѵ�.
* - event�� ������ NO_EVENT�� return�Ѵ�.
* - binding port���� �߻��� ����̸� ���ο� client�� ������ �䱸�� ����̹Ƿ�
*	���ο� client_fd�� �����Ͽ� accept�� �� actFd�� ���ο� client_fd�� �ѱ��,
*	NEW_CONNECTION�� return�Ѵ�.
* - client_fd �Ǵ� server_fd���� �߻��� ����̸� �޽����� ������ ����̰ų� ������
*	������ ����̴�.
*	-> ������ ������ ��� actFd�� disconnect�� fd�� �ѱ��, DISCONNECTED�� return�Ѵ�.
*	-> �޽����� ������ ��� actFd�� �޽����� ������ fd�� �ѱ��, rcvBuf�� ���ŵ�
*      �޽����� ����ϰ�, SOCKLIB_MSG_RECEIVED�� return�Ѵ�.
* - ��Ÿ ���� error�� ��� SOCKLIB_INTERNAL_ERROR�� return�Ѵ�.
------------------------------------------------------------------------------*/
int socklib_action_2 (int *actFd, int *fdTbl_index)
{
	int		port,fd,newFd;

	/* readSockFdSet/exceptSockFdSet�� Ȯ���Ͽ� event �߻� ���θ� Ȯ���Ѵ�.
	*/
	if (socklib_pollFdSet() <= 0)
		return SOCKLIB_NO_EVENT;

	/* binding port���� event�� �����Ǿ����� Ȯ���Ѵ�.
	** - binding port ��ȣ�� binding port�� socket fd�� return�ȴ�.
	*/
	if (socklib_lookupBindSockFdTbl (&port, &fd) > 0) {
		/* ���ο� client_fd�� �����Ѵ�.
		*/
		if ((newFd = socklib_acceptNewConnection (port, fd, fdTbl_index)) < 0) {
			return SOCKLIB_INTERNAL_ERROR;
		}
		*actFd = newFd;
		return SOCKLIB_NEW_CONNECTION;
	}

	/* server�� ����� fd���� event�� �����Ǿ����� Ȯ���Ѵ�.
	*/
	if ((fd = socklib_lookupServerSockFdTbl (fdTbl_index)) > 0) {
		/* ���� �޽����� �ְų� disconnect�� ����ε�, read�� �õ��ؼ� ��������
		**	�޽����� �����ϰ�, read fail�̸� disconnect�� ���̴�.
		*/
		*actFd  = fd;
		if (socklib_readSockFd_2 (&serverSockFdTbl[*fdTbl_index]) < 0) {
			/* read fail�̸� ������ ������ ������ �Ǵ��Ͽ� disconnect�Ѵ�.
			*/
			socklib_disconnectServerFd (fd);
			return SOCKLIB_SERVER_DISCONNECTED;
		}
		return SOCKLIB_SERVER_MSG_RECEIVED;
	}

	/* client_fd���� event�� �����Ǿ����� Ȯ���Ѵ�.
	*/
	if ((fd = socklib_lookupClientSockFdTbl (fdTbl_index)) > 0) {
		/* ���� �޽����� �ְų� disconnect�� ����ε�, read�� �õ��ؼ� ��������
		**	�޽����� �����ϰ�, read fail�̸� disconnect�� ���̴�.
		*/
		*actFd  = fd;
		if (socklib_readSockFd_2 (&clientSockFdTbl[*fdTbl_index]) < 0) {
			/* read fail�̸� ������ ������ ������ �Ǵ��Ͽ� disconnect�Ѵ�.
			*/
			socklib_disconnectClientFd (fd);
			return SOCKLIB_CLIENT_DISCONNECTED;
		}
		return SOCKLIB_CLIENT_MSG_RECEIVED;
	}

	return SOCKLIB_INTERNAL_ERROR;

} /** End of socklib_action_2 **/



/*------------------------------------------------------------------------------
* event�� ������ fd���� �޽����� read�Ͽ� rcvBuf�� �ִ´�.
* - ������ read�ؼ� rcvBuf�� �״´�.
* - rcvBuf�� ������ ���������� Application���� ó������ �ʰ� �����ִ� �޽����� rcvBuf��
*   ���� �� �ִ�. �̹��� ������ �����͸� rcvBuf�� �̾� ���δ�.
------------------------------------------------------------------------------*/
int socklib_readSockFd_2 (SockLibSockFdContext *pFdTbl)
{
	int		len, offset, rcvBuf_freeLen;

	// rcvBuf�� ������ �ް� ���� ó������ �ʴ� �����Ͱ� ���� ���� �� �ְ�,
	//   ���� �ִ� �������� ���̰� rcvMsgLen�̴�.
	// �̹��� ���� �����͸� �� �ڿ� �ٿ��� �ϹǷ� �� offset�� ���� ������.
	// �̹��� �ִ�� ���� �� �ִ� max�� �����Ҷ��� ���� ����ִ� �������� ũ�⿡ ���� �ٸ���.
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
		// �ؿ��� rcvMsgLen�� �����ϹǷ� �̹��� ���� �����ͱ��̰� 0�̶�� �����Ѵ�.
		len = 0;
	}

	// �̹��� �о ������ ���̸�ŭ rcvMsgLen�� update�Ѵ�.
	// �ǳ��� NULL�� ä���.
	//
	offset = pFdTbl->rcvMsgLen += len;
	pFdTbl->rcvBuf[offset] = 0;

	// return���� rcvBuf�� ����ִ� �������� ���̸� return�Ѵ�.
	// ������ �����ִ� �����Ͱ� �ְ� �̹��� �������� �ʾҾ 0���� ū ���� return�ǵ��� �ϱ� ����
	//
#ifdef DEBUG
	//fprintf(stdout,"[socklib_readSockFd_2] read len=%d, ret=%d\n", len, offset);
#endif
	return (offset);

} /** End of socklib_readSockFd_2 **/



/*------------------------------------------------------------------------------
* ������ fd�� �����͸� length��ŭ write�Ѵ�.
* - write�ϱ����� write �������� Ȯ���Ѵ�.
*	- select�� Ȯ���ϴµ� select fail �� fd�� ������ close�Ѵ�.
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
		/* write�� �� �ִ��� Ȯ���Ѵ�.
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
			/* serverSockFdTbl, clientSockFdTbl�� �˻��� server�� ���ӵ� fd����,
			**	client�� ������ fd���� ã�� �� ������ ���´�.
			*/
			socklib_disconnectSockFd (fd);
			return -1;
		}

		/* �ش� fd�� data�� write�Ѵ�.
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
			/* write fail�̸� ������ ������ ���´�.
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
		/* ������ length��ŭ write���� ���� ��� ������ ������ ���´�.
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
* ������ ipAddr,port�� socket fd�� �����Ͽ� �����Ѵ�.
* - ������ fd�� read/except fd_set�� �߰��Ѵ�.
* - ������ fd�� return�ȴ�.
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
	//ualarm ((useconds_t)(100000*socklib_connect_timer), 0); // --->> multi-thread�� ��� ������ �ɼ� �ִ�.
	ualarm (100000*socklib_connect_timer, 0); // --->> multi-thread�� ��� ������ �ɼ� �ִ�.
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

	/* serverSockFdTbl�� address, port, fd�� �����Ѵ�.
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
	//ualarm (100000*socklib_connect_timer, 0); // --->> multi-thread�� ��� ������ �ɼ� �ִ�.
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

	/* serverSockFdTbl�� address, port, fd�� �����Ѵ�.
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
* ������ srcIpAddr,port�� ����, dstIp,port�� socket fd�� �����Ͽ� �����Ѵ�.
* - ������ fd�� read/except fd_set�� �߰��Ѵ�.
* - ������ fd�� return�ȴ�.
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
	ualarm(100000,0); // --->> multi-thread�� ��� ������ �ɼ� �ִ�.
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

	/* serverSockFdTbl�� destIpAddress, destPort, fd�� �����Ѵ�.
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



