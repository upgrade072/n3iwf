#include <dbm.h>

int		gErrLogLevel;
int		myQid, ixpcQid;
char 	myAppName[COMM_MAX_NAME_LEN];

int main()
{
	struct event_base *evbase_main = event_base_new();

	if (initDbm() < 0) {
		exit(-1);
	}

	struct timeval one_sec = {1, 0};
	struct event *ev_keepalive = event_new(evbase_main, -1, EV_PERSIST, main_tick, NULL);
	event_add(ev_keepalive, &one_sec);

	struct timeval mil_sec = {0, 1000};
	struct event *ev_msgq_read = event_new(evbase_main, -1, EV_PERSIST, checkDbmMsgQueue, NULL);
	event_add(ev_msgq_read, &mil_sec);

	event_base_loop(evbase_main, EVLOOP_NO_EXIT_ON_EMPTY);

	return 0;
}

void main_tick(evutil_socket_t fd, short events, void *data)
{
	keepalivelib_increase();
}

int initDbm()
{
	strcpy(myAppName, "DBM");

	if (conflib_initConfigData() < 0) {
		fprintf(stderr, "[%s.%d] conflib_initConfigData() Error\n", FL);
		return -1;
	}

	if (keepalivelib_init(myAppName) < 0) {
		fprintf(stderr, "[%s.%d] keepalivelib_init() Error\n", FL);
		return -1;
	}

	if (initLog(myAppName) < 0) {
		fprintf(stderr, "[%s.%d] initLog() Error\n", FL);
		return -1;
	}

	if (initMsgQ() < 0) {
		fprintf(stderr, "[%s.%d] initMsgQ() Error\n", FL);
		return -1;
	}

	if (initMmc() < 0) {
		fprintf(stderr, "[%s.%d] initMmc() Error\n", FL);
		return -1;
	}

	if (msgtrclib_init() < 0) {
		fprintf(stderr, "[%s.%d] msgtrclib() Error\n", FL);
		return -1;
	}

	STARTLOG();

	return 0;
}

int initConf()
{
    char    confBuf[10240];

    memset(confBuf, 0, sizeof(confBuf));

    if (loadAppConfig(confBuf) < 0) {
        fprintf(stderr, "[%s.%d] loadAppConfig[%s]\n", FL, confBuf);
        return -1;
    }

    printAppConfig(confBuf);

    ERRLOG(LLE, FL, "\n%s\n", confBuf);

    return 0;
}

void printAppConfig(char* resBuf)
{
	int len = 0;

	/* log level */
	len += sprintf(&resBuf[len], "  LOG_LEVEL :\n");
	len += sprintf(&resBuf[len], "   %-22s = [%d]\n", "ERR_LOG_LEVEL"   , loglib_getLogLevel(ELI));

	return;
}

int loadAppConfig(char* resBuf)
{
	char 	fname[128], buf[256];

	//SYSCONF_FILE
	sprintf(fname, "%s/%s/%s.dat", homeEnv, APPCONF_DIR, myAppName);

	if(conflib_getNthTokenInFileSection(fname, "[GENERAL]", "ERR_LOG_LEVEL", 1, buf) < 0) {
		sprintf(resBuf, "ERR_LOG_LEVEL FIELD NOT FOUND IN %s", fname);
		return -1;
	}
	loglib_setLogLevel(ELI, atoi(buf));
	gErrLogLevel = loglib_getLogLevel(ELI);

	printAppConfig(resBuf);
	return 1;
}

int initMsgQ(void)
{
	/** MY QID **/
	if ((myQid = commlib_crteMsgQ(l_sysconf, myAppName, TRUE))<0 ) {
		fprintf(stderr, "[%s.%d] [%s] error, crteMsgQ(%s) fail\n", FL, __func__, myAppName);
		return -1;
	}   

	/** IXPC QID **/
	if ((ixpcQid = commlib_crteMsgQ(l_sysconf, "IXPC", FALSE))<0 ) {
		fprintf(stderr, "[%s.%d] [%s] error, crteMsgQ(IXPC) failed\n", FL, __func__);
		return -1;
	}

	return 0;
}

void checkDbmMsgQueue(evutil_socket_t fd, short events, void *data)
{
	int             mCnt=0;
	GeneralQMsgType rcvMsg;

	while (1) {
		if (msgrcv (myQid, &rcvMsg, sizeof(GeneralQMsgType), 0, IPC_NOWAIT) < 0) {
			if(errno != ENOMSG) {
				ERRLOG(LL3, FL, "[%s] msgrcv failed. (errno=%d:%s)\n", __func__, errno, strerror(errno));
			}
			return;
		}

		mCnt++;
		switch (rcvMsg.mtype) {
			case MTYPE_SETPRINT:
				trclib_exeSetPrintMsg ((TrcLibSetPrintMsgType*)&rcvMsg);
				break;
			case MTYPE_MMC_REQUEST:
				handleMMCReq((IxpcQMsgType *)rcvMsg.body);
				break;
			case MTYPE_RELOAD_CONFIG_DATA:
				mmcReloadConfigData((IxpcQMsgType*) rcvMsg.body);
				break;
			case MTYPE_DIS_CONFIG_DATA:
				mmcDisConfigData((IxpcQMsgType*) rcvMsg.body);
				break;
			default:
				ERRLOG(LL3, FL, "[%s] Unknown mtype is received. mtype=%ld\n", __func__, rcvMsg.mtype);
				break;
		}

		if (mCnt > 50) {
			break;
		}
	}

	return;
}

