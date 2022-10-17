#include <commlib.h>
#include <stddef.h>

/* 
 * trcLogId, trcErrLogId, msgLogId, cdrLogId
 * 각각의 APP BLOCK에서 재정의 하지 말 것 
 * loglib.h 를 include하여 사용할 것 
 */
int     trcLogId;       /* debug_log 용으로 사용 */
int     trcErrLogId;    /* error_log 용으로 사용 */
int     trcMsgLogId;    /* 연동메시지 로그로 사용 */
int     cdrLogId;       /* cdr_log 용으로 사용 */

static LogInformationTable	*log_tbl;			/* log table SHM */
static LogInformationTable	gLogTbl;			/* For not registered APPLICATIONS. TOOL, etc... */
#ifdef _SHM_LOGPROP
LogInformationTable			*myLogTbl = NULL;	/* 블록별 log table 정보 */
#else
LogInformationTable			*myLogTbl = &gLogTbl;	/* 블록별 log table 정보 */
#endif
struct log_level_info		*shm_loglvl = NULL;

static pthread_mutex_t loglibLock = PTHREAD_MUTEX_INITIALIZER;

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
//
// 로그를 파일을 생성하고, 해당 파일에 text로 로그를 기록하는 기능을 제공하는 library이다.
//
// loglib_openLog()를 호출할때 원하는 파일의 이름과 파일의 mode를 지정할 수 있다.
//
// mode는 크게 나누어
// 1. text의 앞에 로그기록 시각을 포함하거나 파일의 이름과 라인수를 포함하는 option과
// 2. 파일의 생성 원칙을 지정하는 option으로 나누어진다.
//
// LOGLIB_TIME_STAMP : text 앞에 현재 시각을 [년-월-일 시:분:초.msec] format으로 추가한다.
//                     ex) [2007-06-21 22:41:11.421] this is log message
//
// LOGLIB_FNAME_LNUM : text 앞에 lib를 호출한 파일의 이름과 라인수 (__FILE__, __LINE___)를 추가한다.
//                     ex) [test.c:138] this is log message
//
// LOGLIB_MODE_DAILY : 파일을 날짜별로 기록하는 option으로
//                     - 지정된 파일을 directory부와 file부로 나누어 directory_name의 sub directory에
//                     - 현재날짜에 해당하는 directory를 생성하고 file_name에 해당하는 파일을 기록한다.
//                     - 파일 기록 중 날짜가 변경되면 자동으로 기존 파일의 닫고,
//                       새로운 날짜에 해당하는 directory를 다시 생성하고 file_name에 해당파일에 기록한다.
// LOGLIB_MODE_HOURLY : 파일을 시간대별로 기록하는 option으로
//                     - 지정된 파일을 directory부와 file부로 나누어 directory_name의 sub directory에
//                     - 현재시간에 해당하는 directory를 생성하고 file_name.mmddhh에 해당하는 파일을 기록한다.
//                     - 파일 기록 중 시간대가 변경되면 자동으로 기존 파일의 닫고,
//                       새로운 시간대에 해당하는 directory와 file을 다시 생성하고 해당파일에 기록한다.
//
// LOGLIB_MODE_LIMIT_SIZE : 파일 최대 크기를 제한하는 option으로
//                          - 지정된 파일의 끝에 숫자 0을 붙이고, 최대크기를 초고하면 기존파일을 닫고
//                            기존파일을 다른이름으로 rename한 후 다시 파일을 열어 기록한다.
//                            파일끝에 붙는 숫자는 0 ~ max까지 가능하며 .0 파일이 현재 기록되고 있는 파일이다.
//                          ex) fname = "/tmp/log_dir/log_file"
//                              --> /tmp/log_dir/log_file.0 의 파일에 기록된다.
//                          - 한 파일의 최대 크기와 끝첨자를 몇개까지 관리할 건지 property에 설정하지 않으면
//                            loglib.h에 있는 default 값으로 설정된다.
//
// 모든 option은 "|"를 이용하여 중복 지정할 수 있으나
// - 단, LOGLIB_MODE_DAILY와 LOGLIB_MODE_HOURLY는 동시에 지정할 수 없다.
//
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------

int system_popen(char *cmd)
{
	FILE    *fp;
	char    buf[1024];
	if ((fp = popen (cmd, "r")) == NULL) {
		fprintf (stderr, "[%s] popen fail; cmd=[%s] errno=%d(%s)\n",__func__,cmd,errno,strerror(errno));
		return -1;
	}
	while (fgets (buf, sizeof(buf), fp) != NULL) { ; }
	pclose (fp);
	return 1;
}

#ifdef _SHM_LOGPROP
int loglib_initLog (char *appName)
{
    int     err;
	int		shmid;
	int 	appId;
    char    *env;
	char	getBuf[256], token[64];
	FILE	*fp;

	myLogTbl = NULL;

	if ((env = getenv(IV_HOME)) == NULL) {
		fprintf(stderr, "[%s] not found %s environment name\n", __func__, IV_HOME);
		return -1;
	}

	sprintf(l_sysconf, "%s/%s", env, SYSCONF_FILE);

    pthread_mutex_lock (&loglibLock);

	size_t shm_size = sizeof(LogInformationTable)*LOG_PROC_MAX;
	if ((err=commlib_crteShm(l_sysconf, SHM_LOGPROP, shm_size, (void **)&log_tbl))<0 ) {
        fprintf(stderr,"[%s] shmget fail(%s); size=%ld: err=%d[%s]\n", 
				__func__, SHM_LOGPROP, (unsigned long)shm_size,  errno, strerror(errno));
		pthread_mutex_unlock (&loglibLock);
        return -1;
	}

    pthread_mutex_unlock (&loglibLock);

	if ( err==SHM_CREATE )
		memset(log_tbl, 0x00, shm_size);

	if((fp = fopen(l_sysconf,"r")) == NULL) {
		fprintf(stderr,"[%s] fopen fail[%s]; err=%d(%s)\n", __func__, l_sysconf, errno, strerror(errno));
		return -1;
	}

	if (conflib_seekSection (fp, "APPLICATIONS") < 0 ) {
		fprintf(stderr,"[%s] conflib_seekSection(APPLICATIONS) fail\n", __func__);
		fclose(fp);
		return -1;
	}

	appId = 0;
	while (fgets(getBuf,sizeof(getBuf),fp) != NULL) {
		if (getBuf[0] == '[') /* end of section */
			break;
		if (getBuf[0]=='#' || getBuf[0]=='\n') /* comment line or empty */
			continue;

		sscanf (getBuf,"%63s",token);
		if (!strcasecmp(token,appName)) {
			sprintf(log_tbl[appId].appName, "%s", token);
			myLogTbl = (LogInformationTable*)&log_tbl[appId];
			memset(myLogTbl->logInfo, 0, sizeof(LogInformation)*MAX_APPLOG_NUM);

			fclose(fp);
			return appId;
		}
		
		if(++appId >= LOG_PROC_MAX) {
			break;
		}
	}

__TOOL_LOG:
	/* For not registered applications. Tool, etc... */
	strcpy(gLogTbl.appName, appName);
	myLogTbl = (LogInformationTable*)&gLogTbl;
	memset(myLogTbl->logInfo, 0, sizeof(LogInformation)*MAX_APPLOG_NUM);
	/**/

	fclose(fp);
	return 0; //-2;
}
#else /* _SHM_LOGPROP */
int loglib_initLog (char *appName)
{
    //int     err;
    //char    *env;

	//myLogTbl = NULL;

	//if ((env = getenv(IV_HOME)) == NULL) {
	//	fprintf(stderr, "[%s] not found %s environment name\n", __func__, IV_HOME);
	//	return -1;
	//}

	//sprintf(l_sysconf, "%s/%s", env, SYSCONF_FILE);

    //pthread_mutex_lock (&loglibLock);

	//size_t shm_size = sizeof(LogInformationTable) * LOG_PROC_MAX;
	//if ((err=commlib_crteShm(l_sysconf, SHM_LOGPROP, shm_size, (void **)&log_tbl))<0 ) {
	//    fprintf(stderr,"[%s] shmget fail(%s); size=%ld: err=%d[%s]\n", 
	//			__func__, SHM_LOGPROP, shm_size,  errno, strerror(errno));
	//	pthread_mutex_unlock (&loglibLock);
	//    return -1;
	//}

    //pthread_mutex_unlock (&loglibLock);

	//if ( err==SHM_CREATE )
	//	memset(log_tbl, 0x00, shm_size);

	//myLogTbl = (LogInformationTable*)&log_tbl[proc_index];
	memset (myLogTbl, 0x00, sizeof (LogInformationTable));
	
	sprintf (myLogTbl->appName, "%s", appName);

	return 0;
}
#endif /* _SHM_LOGPROP */


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int loglib_openLog (LoglibProperty *pro)
{
	struct timeval	curr;
    int     		i, logId;
    char    		tmp[256];

    if (myLogTbl == NULL) {
        fprintf (stderr, "[%s] myLogTbl is null \n", __func__);
        return -1;
    }

    if (pro->fname == NULL) {
        fprintf (stderr, "[%s] log file name is null \n", __func__);
        return -1;
    }

    if (pro->appName[0] == '\0') {
        fprintf (stderr, "[%s] app name is null \n", __func__);
        return -1;
    }

    // DAILY 모드와 HOURLY 모드를 중복해서 지정할 수 없다.
    if ((pro->mode & LOGLIB_MODE_DAILY) && (pro->mode & LOGLIB_MODE_HOURLY)) {
        fprintf (stderr, "[%s] invalid log_mode; (conflict DAILY and HOURLY)\n", __func__);
        return -1;
    }

    for (i=0; i<MAX_APPLOG_NUM; i++) {
        if (myLogTbl->logInfo[i].fp == NULL) {
            logId = i;
            break;
        }
    }
    if (i == MAX_APPLOG_NUM) {
        fprintf (stderr, "[%s] can't open file any more... MAX_APPLOG_NUM=[%d] \n",
				__func__, MAX_APPLOG_NUM);
        return -1;
    }


    // property 저장
    memcpy (&myLogTbl->logInfo[logId].pro, pro, sizeof(LoglibProperty));

    // 허용 값 범위 검사 및 default 설정
    if (pro->num_suffix < LOGLIB_NUM_SUFFIX_MIN || pro->num_suffix > LOGLIB_NUM_SUFFIX_MAX)
        myLogTbl->logInfo[logId].pro.num_suffix = LOGLIB_DEFAULT_NUM_SUFFIX;
    if (pro->limit_val < LOGLIB_SIZE_LIMIT_MIN || pro->limit_val > LOGLIB_SIZE_LIMIT_MAX)
        myLogTbl->logInfo[logId].pro.limit_val = LOGLIB_DEFAULT_SIZE_LIMIT;

    // subdir_format 검사. strftime()을 호출해 봄으로써 유효성 검사
    if (!strcmp(pro->subdir_format,"")) {
        if (pro->mode & LOGLIB_MODE_DAILY || pro->mode & LOGLIB_MODE_HOURLY)
            strcpy (myLogTbl->logInfo[logId].pro.subdir_format, "%Y-%m-%d");
    } else {
	    gettimeofday (&curr, NULL);
        if (strftime (tmp, sizeof(tmp), pro->subdir_format, localtime(&curr.tv_sec)) == 0) {
            fprintf (stderr, "[%s] invalid subdir_format(%s)... strftime() failed \n", __func__, pro->subdir_format);
            return -1;
        }
    }
    myLogTbl->logInfo[logId].fp       = NULL;
    myLogTbl->logInfo[logId].lastTime = 0;


    // LOGLIB_MODE_DAILY인 경우, 오늘날짜의 파일(log_dir/log_file_yyyymmdd) append 모드로
    //	open하고 myLogTbl->logInfo[logId].fp에 저장되어 return된다.
    // - LOGLIB_MODE_nDAYS인 경우 n일전 파일을 지운다.
    //
    if (pro->mode & LOGLIB_MODE_DAILY) {
        if (loglib_checkDate(logId) < 0)
            return -1;
        myLogTbl->logInfo[logId].lastTime = time(0);
    }

    // LOGLIB_MODE_HOURLY인 경우, 현재시각 이름의 파일(log_dir/yyyy-mm-dd/log_file.mmddhh) append 모드로
    //	open하고 myLogTbl->logInfo[logId].fp에 저장되어 return된다.
    // - directory가 없으면 먼저 생성한 후 파일을 생성한다.
    //
    if (pro->mode & LOGLIB_MODE_HOURLY) {
        if (loglib_checkTimeHour(logId) < 0)
            return -1;
        myLogTbl->logInfo[logId].lastTime = time(0);
    }

    // LOGLIB_MODE_LIMIT_SIZE인 경우 "xxxx.0" 파일의 유무를 확인하고,
    // - 없으면 생성한 후 myLogTbl->logInfo[logId].fp에 저장되어 return되고,
    // - 있으면 limit 초과 여부를 확인하여 초과하지 않았으면 append 모드로 open한 후
    //	myLogTbl->logInfo[logId].fp에 저장되어 return되고,
    // - 파일이 존재하고 limit를 초과한 경우 기존파일을 rename한 후 "xxxx.0"를
    //	다시 생성하여 myLogTbl->logInfo[logId].fp에 저장하고 return된다.
    //
    if (pro->mode & LOGLIB_MODE_LIMIT_SIZE) {
        if (loglib_checkLimitSize(logId) < 0)
            return -1;
    }

    if(myLogTbl->logInfo[logId].fp == NULL) {
		if(loglib_makeDefaultFile(logId) < 0)
			return -1;
	}

    return logId;

} //-- End of loglib_openLog --//

int loglib_makeDefaultFile(int logId)
{
    char cmd[256];
	char newFile[256];
	char log_dir[256], log_file[64];

	strcpy(newFile, myLogTbl->logInfo[logId].pro.fname);

	if ((myLogTbl->logInfo[logId].fp = fopen(newFile, "a+")) == NULL) {
		if (errno != ENOENT) {
			fprintf(stderr,"[%s] fopen fail[%s]; errno=%d(%s)\n",__func__,newFile,errno,strerror(errno));
			return -1;
		}
		// full name인 fname을 directory부와 file name부로 나눈다.
		commlib_getLastTokenInString (newFile, "/", log_file, log_dir);
		log_dir[strlen(log_dir)-1] = 0;
		sprintf (cmd, "/bin/mkdir -p %s", log_dir);
		system_popen (cmd);
		if ((myLogTbl->logInfo[logId].fp = fopen(newFile,"a+")) == NULL) {
			fprintf(stderr,"[%s] fopen fail[%s]; errno=%d(%s)\n",__func__,newFile,errno,strerror(errno));
			return -1;
		}
	}
	return 1;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int loglib_closeLog (int logId)
{
	if (myLogTbl->logInfo[logId].fp != NULL) {
		fflush (myLogTbl->logInfo[logId].fp);
		fclose (myLogTbl->logInfo[logId].fp);
	}
	myLogTbl->logInfo[logId].fp = NULL;
	return logId;
} //-- End of loglib_closeLog --//

int loglib_setLogLevel(int logId, int level) 
{
	if( logId < 0 || logId >= MAX_APPLOG_NUM ) {
		fprintf(stderr,"[%s] invalid logId(%d)\n", __func__, logId);
		return -1;
	}
	myLogTbl->logInfo[logId].level = level;

	return 0;
}       

int loglib_getLogLevel(int logId)
{
	if( logId < 0 || logId>= MAX_APPLOG_NUM ) {
		fprintf(stderr,"[%s] invalid logId(%d)\n", __func__, logId);
		return -1;
	}
	return myLogTbl->logInfo[logId].level;
}

const char* loglib_getLogLevelStr(int logId)
{
	int level = loglib_getLogLevel (logId);
	if (level >= LL3)
		return "DETAIL";
	else if (level >= LL2)
		return "SIMPLE";
	else if (level >= LL1)
		return "ERROR";
	else 
		return "OFF";
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int logPrint ( int logId, char *fName, int lNum, char*fmt, ...)
{
	int		ret;
	char   	buff[TRCBUF_LEN], optBuf[256], tmp[64];
	va_list	args;
	struct timeval	curr;

    pthread_mutex_lock (&loglibLock);

    va_start(args, fmt);
    (void)vsprintf(buff, fmt, args);
    va_end(args);

	strcpy (optBuf, "");

	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_DAILY)
	{
		// 현재 열려있는 파일정보와 현재 시각을 비교하여 날짜가 바뀌었는지 확인한다.
		// - 날짜가 바뀌었으면 오늘 날짜 파일의 연다.
		// - LOGLIB_MODE_nDAYS인 경우 n일전 파일을 지운다.
		//
		if (loglib_checkDate(logId) < 0)
			goto __FAIL_RETURN;
	}
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_HOURLY)
	{
		// 현재 열려있는 파일정보와 현재 시각을 비교하여 시간(hour)이 바뀌었는지 확인한다.
		// - 시간이 바뀌었으면 현재 시간 파일의 연다.
		// - 년-월-일 directory 구조 밑에 파일이 생성되므로 directory가 없으면 먼저 만든다.
		//
		if (loglib_checkTimeHour(logId) < 0)
			goto __FAIL_RETURN;
	}
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_LIMIT_SIZE)
	{
		// 현재 열려있는 파일이 limit를 초과했는지 확인한다.
		// - 초과된 경우 이전파일을 rename하고 다시 연다.
		//
		if (loglib_checkLimitSize(logId) < 0)
			goto __FAIL_RETURN;
	}
	if ( myLogTbl->logInfo[logId].fp == NULL) {
		goto __FAIL_RETURN;
	}

	gettimeofday (&curr, NULL);

	// LOGLIB_TIME_STAMP이면 현재 시각을 맨 앞에 함께 기록한다.
    //
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_TIME_STAMP) {
		strftime(tmp, 32, "%m-%d %T", localtime(&curr.tv_sec));
		sprintf (optBuf, "[%s.%03d] ", tmp, (int)(curr.tv_usec/1000));
		/*sprintf (optBuf, "[%s.%06d] ", tmp, (int)curr.tv_usec);*/
	}
	// LOGLIB_FNAME_LNUM이면 소스 파일 이름과 라인 수를 함께 기록한다.
    //
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_FNAME_LNUM) {
		sprintf(tmp, "[%s:%04d] ", fName, lNum);
		strcat (optBuf, tmp);
	}

	if (myLogTbl->logInfo[logId].fp != NULL) {
		ret = fprintf (myLogTbl->logInfo[logId].fp, "%s%s", optBuf, buff);
		myLogTbl->logInfo[logId].lastTime = curr.tv_sec;
	} else {
		goto __FAIL_RETURN;
	}

	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_FLUSH_IMMEDIATE) {
		fflush (myLogTbl->logInfo[logId].fp);
	}

    pthread_mutex_unlock (&loglibLock);

	return ret;

__FAIL_RETURN:
    pthread_mutex_unlock (&loglibLock);
    return -1;

} //-- End of logPrint --//


//----------------- 사용중지 ---------------------------------------------------
//------------------------------------------------------------------------------
#if 0
int _LogPrint ( int logId, char *fName, char *func, int lNum, char*fmt, ...)
{
	int		ret;
	char   	buff[TRCBUF_LEN], optBuf[256], tmp[64];
	va_list	args;
	struct timeval	curr;

    pthread_mutex_lock (&loglibLock);

    va_start(args, fmt);
    (void)vsprintf(buff, fmt, args);
    va_end(args);

	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_DAILY)
	{
		// 현재 열려있는 파일정보와 현재 시각을 비교하여 날짜가 바뀌었는지 확인한다.
		// - 날짜가 바뀌었으면 오늘 날짜 파일의 연다.
		// - LOGLIB_MODE_nDAYS인 경우 n일전 파일을 지운다.
		//
		if (loglib_checkDate(logId) < 0)
			goto __FAIL_RETURN;
	}
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_HOURLY)
	{
		// 현재 열려있는 파일정보와 현재 시각을 비교하여 시간(hour)이 바뀌었는지 확인한다.
		// - 시간이 바뀌었으면 현재 시간 파일의 연다.
		// - 년-월-일 directory 구조 밑에 파일이 생성되므로 directory가 없으면 먼저 만든다.
		//
		if (loglib_checkTimeHour(logId) < 0)
			goto __FAIL_RETURN;
	}
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_LIMIT_SIZE)
	{
		// 현재 열려있는 파일이 limit를 초과했는지 확인한다.
		// - 초과된 경우 이전파일을 rename하고 다시 연다.
		//
		if (loglib_checkLimitSize(logId) < 0)
			goto __FAIL_RETURN;
	}
	if ( myLogTbl->logInfo[logId].fp == NULL) {
		goto __FAIL_RETURN;
	}

	gettimeofday (&curr, NULL);
	strcpy (optBuf, "");

	// LOGLIB_TIME_STAMP이면 현재 시각을 맨 앞에 함께 기록한다.
    //
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_TIME_STAMP) {
		strftime(tmp, 32, "%m-%d %T", localtime(&curr.tv_sec));
		sprintf (optBuf, "[%s.%03d] ", tmp, (int)(curr.tv_usec/1000));
		/*sprintf (optBuf, "[%s.%06d] ", tmp, (int)curr.tv_usec);*/
	}
	// LOGLIB_FNAME_LNUM이면 소스 파일 이름과 라인 수를 함께 기록한다.
    //
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_FNAME_LNUM) {
		sprintf(tmp, "[%s:%s:%04d] ", fName, func, lNum);
		strcat (optBuf, tmp);
	}

	if (myLogTbl->logInfo[logId].fp != NULL) {
		ret = fprintf (myLogTbl->logInfo[logId].fp, "%s%s", optBuf, buff);
		myLogTbl->logInfo[logId].lastTime = curr.tv_sec;
	} else {
		goto __FAIL_RETURN;
	}

	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_FLUSH_IMMEDIATE) {
		fflush (myLogTbl->logInfo[logId].fp);
	}

    pthread_mutex_unlock (&loglibLock);

	return ret;

__FAIL_RETURN:
    pthread_mutex_unlock (&loglibLock);
    return -1;

} //-- End of logPrint --//
#endif

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int __lprintf (int logId, int level, const char *fName, int lNum, char*fmt, ...)
{
	int		ret;
	char   	buff[TRCBUF_LEN], optBuf[256], tmp[64];
	va_list	args;
	struct timeval	curr;

#if 0
	if(log_tbl[logId].level < level ) {
		return 0;
	}
#endif

    pthread_mutex_lock (&loglibLock);

    va_start(args, fmt);
    (void)vsprintf(buff, fmt, args);
    va_end(args);

	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_DAILY)
	{
		// 현재 열려있는 파일정보와 현재 시각을 비교하여 날짜가 바뀌었는지 확인한다.
		// - 날짜가 바뀌었으면 오늘 날짜 파일의 연다.
		// - LOGLIB_MODE_nDAYS인 경우 n일전 파일을 지운다.
		//
		if (loglib_checkDate(logId) < 0)
			goto __FAIL_RETURN;
	}
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_HOURLY)
	{
		// 현재 열려있는 파일정보와 현재 시각을 비교하여 시간(hour)이 바뀌었는지 확인한다.
		// - 시간이 바뀌었으면 현재 시간 파일의 연다.
		// - 년-월-일 directory 구조 밑에 파일이 생성되므로 directory가 없으면 먼저 만든다.
		//
		if (loglib_checkTimeHour(logId) < 0)
			goto __FAIL_RETURN;
	}
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_LIMIT_SIZE)
	{
		// 현재 열려있는 파일이 limit를 초과했는지 확인한다.
		// - 초과된 경우 이전파일을 rename하고 다시 연다.
		//
		if (loglib_checkLimitSize(logId) < 0)
			goto __FAIL_RETURN;
	}
	if ( myLogTbl->logInfo[logId].fp == NULL) {
		goto __FAIL_RETURN;
	}

	gettimeofday (&curr, NULL);
	strcpy (optBuf, "");

	// LOGLIB_TIME_STAMP이면 현재 시각을 맨 앞에 함께 기록한다.
    //
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_TIME_STAMP) {
		strftime(tmp, 32, "%m-%d %T", localtime(&curr.tv_sec));
		/*sprintf (optBuf, "[%s.%03d] ", tmp, (int)(curr.tv_usec/1000));*/
		sprintf (optBuf, "[%s.%06d] ", tmp, (int)curr.tv_usec);
	}
	// LOGLIB_FNAME_LNUM이면 소스 파일 이름과 라인 수를 함께 기록한다.
    //
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_FNAME_LNUM) {
		//sprintf(tmp, "[%s:%04d] [%s] ", fName, lNum, STRLOGLVL[level]);
		sprintf(tmp, "[%s:%04d] ", fName, lNum);
		strcat (optBuf, tmp);
	}

	if (myLogTbl->logInfo[logId].fp != NULL) {
		ret = fprintf (myLogTbl->logInfo[logId].fp, "%s%s", optBuf, buff);
		myLogTbl->logInfo[logId].lastTime = curr.tv_sec;
	} else {
		goto __FAIL_RETURN;
	}

	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_FLUSH_IMMEDIATE) {
		fflush (myLogTbl->logInfo[logId].fp);
	}

    pthread_mutex_unlock (&loglibLock);

	return ret;

__FAIL_RETURN:
    pthread_mutex_unlock (&loglibLock);
    return -1;

} //-- End of lprintf --//


int __cdrprintf (int logId, int level, const char *fName, int lNum, char*fmt, ...)
{
	int		ret;
	char   	buff[TRCBUF_LEN], optBuf[256], tmp[64];
	va_list	args;
	struct timeval	curr;

#if 0
	if(log_tbl[logId].level < level ) {
		return 0;
	}
#endif

    pthread_mutex_lock (&loglibLock);

    va_start(args, fmt);
    (void)vsprintf(buff, fmt, args);
    va_end(args);

	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_DAILY)
	{
		// 현재 열려있는 파일정보와 현재 시각을 비교하여 날짜가 바뀌었는지 확인한다.
		// - 날짜가 바뀌었으면 오늘 날짜 파일의 연다.
		// - LOGLIB_MODE_nDAYS인 경우 n일전 파일을 지운다.
		//
		if (loglib_checkDate(logId) < 0)
			goto __FAIL_RETURN;
	}
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_HOURLY)
	{
		// 현재 열려있는 파일정보와 현재 시각을 비교하여 시간(hour)이 바뀌었는지 확인한다.
		// - 시간이 바뀌었으면 현재 시간 파일의 연다.
		// - 년-월-일 directory 구조 밑에 파일이 생성되므로 directory가 없으면 먼저 만든다.
		//
		if (loglib_checkTimeHour(logId) < 0)
			goto __FAIL_RETURN;
	}
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_LIMIT_SIZE)
	{
		// 현재 열려있는 파일이 limit를 초과했는지 확인한다.
		// - 초과된 경우 이전파일을 rename하고 다시 연다.
		//
		if (loglib_checkLimitSize(logId) < 0)
			goto __FAIL_RETURN;
	}
	if ( myLogTbl->logInfo[logId].fp == NULL) {
		goto __FAIL_RETURN;
	}

	gettimeofday (&curr, NULL);
	strcpy (optBuf, "");

	// LOGLIB_TIME_STAMP이면 현재 시각을 맨 앞에 함께 기록한다.
    //
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_TIME_STAMP) {
		strftime(tmp, 32, "%m-%d %T", localtime(&curr.tv_sec));
		/*sprintf (optBuf, "[%s.%03d] ", tmp, (int)(curr.tv_usec/1000));*/
		sprintf (optBuf, "[%s.%06d] ", tmp, (int)curr.tv_usec);
	}
	// LOGLIB_FNAME_LNUM이면 소스 파일 이름과 라인 수를 함께 기록한다.
    //
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_FNAME_LNUM) {
		//sprintf(tmp, "[%s:%04d] [%s] ", fName, lNum, STRLOGLVL[level]);
		sprintf(tmp, "[%s:%04d] ", fName, lNum);
		strcat (optBuf, tmp);
	}

	if (myLogTbl->logInfo[logId].fp != NULL) {
		ret = fprintf (myLogTbl->logInfo[logId].fp, "%s%s", optBuf, buff);
		myLogTbl->logInfo[logId].lastTime = curr.tv_sec;
	} else {
		goto __FAIL_RETURN;
	}

	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_FLUSH_IMMEDIATE) {
		fflush (myLogTbl->logInfo[logId].fp);
	}

    pthread_mutex_unlock (&loglibLock);

	return ret;

__FAIL_RETURN:
    pthread_mutex_unlock (&loglibLock);
    return -1;

} //-- End of lprintf --//

/* __lprintf와 동일함. 단, color print 수행함 */
int __cprintf (int logId, int level, const char *fName, int lNum, char*fmt, ...)
{
	int		ret;
	char   	buff[TRCBUF_LEN], optBuf[256], tmp[64];
	va_list	args;
	struct timeval	curr;

#if 0
	if(log_tbl[logId].level < level ) {
		return 0;
	}
#endif

    pthread_mutex_lock (&loglibLock);

    va_start(args, fmt);
    (void)vsprintf(buff, fmt, args);
    va_end(args);

	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_DAILY)
	{
		// 현재 열려있는 파일정보와 현재 시각을 비교하여 날짜가 바뀌었는지 확인한다.
		// - 날짜가 바뀌었으면 오늘 날짜 파일의 연다.
		// - LOGLIB_MODE_nDAYS인 경우 n일전 파일을 지운다.
		//
		if (loglib_checkDate(logId) < 0)
			goto __FAIL_RETURN;
	}
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_HOURLY)
	{
		// 현재 열려있는 파일정보와 현재 시각을 비교하여 시간(hour)이 바뀌었는지 확인한다.
		// - 시간이 바뀌었으면 현재 시간 파일의 연다.
		// - 년-월-일 directory 구조 밑에 파일이 생성되므로 directory가 없으면 먼저 만든다.
		//
		if (loglib_checkTimeHour(logId) < 0)
			goto __FAIL_RETURN;
	}
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_LIMIT_SIZE)
	{
		// 현재 열려있는 파일이 limit를 초과했는지 확인한다.
		// - 초과된 경우 이전파일을 rename하고 다시 연다.
		//
		if (loglib_checkLimitSize(logId) < 0)
			goto __FAIL_RETURN;
	}
	if ( myLogTbl->logInfo[logId].fp == NULL) {
		goto __FAIL_RETURN;
	}

	gettimeofday (&curr, NULL);
	strcpy (optBuf, "");

	// LOGLIB_TIME_STAMP이면 현재 시각을 맨 앞에 함께 기록한다.
    //
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_TIME_STAMP) {
		strftime(tmp, 32, "%m-%d %T", localtime(&curr.tv_sec));
		/*sprintf (optBuf, "[%s.%03d] ", tmp, (int)(curr.tv_usec/1000));*/
		sprintf (optBuf, "[%s.%06d] ", tmp, (int)curr.tv_usec);
	}
	// LOGLIB_FNAME_LNUM이면 소스 파일 이름과 라인 수를 함께 기록한다.
    //
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_FNAME_LNUM) {
		//sprintf(tmp, "[%s:%04d] [%s] ", fName, lNum, STRLOGLVL[level]);
		sprintf(tmp, "[%s:%04d] ", fName, lNum);
		strcat (optBuf, tmp);
	}

	// color print
	if (myLogTbl->logInfo[logId].fp != NULL) {
		if(level == LL1)
			ret = fprintf (myLogTbl->logInfo[logId].fp, "%s"DRED("%s")"\n", optBuf, buff);
		else if(level == LL2)
			ret = fprintf (myLogTbl->logInfo[logId].fp, "%s"DYELLOW("%s")"\n", optBuf, buff);
		else if(level == LL3)
			ret = fprintf (myLogTbl->logInfo[logId].fp, "%s%s\n", optBuf, buff);
		else if(level == LL4)
			ret = fprintf (myLogTbl->logInfo[logId].fp, "%s"DGREEN("%s")"\n", optBuf, buff);
		else if(level == LL4)
			ret = fprintf (myLogTbl->logInfo[logId].fp, "%s"DCYAN("%s")"\n", optBuf, buff);
		else
			ret = fprintf (myLogTbl->logInfo[logId].fp, "%s%s\n", optBuf, buff);

		myLogTbl->logInfo[logId].lastTime = curr.tv_sec;
	} else {
		goto __FAIL_RETURN;
	}

	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_FLUSH_IMMEDIATE) {
		fflush (myLogTbl->logInfo[logId].fp);
	}

    pthread_mutex_unlock (&loglibLock);

	return ret;

__FAIL_RETURN:
    pthread_mutex_unlock (&loglibLock);
    return -1;

} //-- End of lprintf --//





//----------------------------------------------------------
// yyyymmdd 를 int 로 return
//----------------------------------------------------------
int loglib_getDay (time_t when)
{
    struct tm       pLocalTime;
	char	szTime[32];

	memset(szTime,	0x00,	sizeof(szTime));
    if ( localtime_r((time_t*)&when, &pLocalTime) == (struct tm *) NULL) {
        strcpy (szTime,"0");
    } else {
        strftime (szTime, 32, "%Y%m%d", &pLocalTime);
    }

    return (atoi(szTime));
} 

//------------------------------------------------------------------------------
// LOGLIB_MODE_DAILY인 경우 날짜가 바뀌었는지 확인한다.
// - 날짜가 바뀐 경우 오늘 날짜 파일을 연다.
//------------------------------------------------------------------------------
int loglib_checkDate (int logId)
{
	char	newFile[256], dirName[256], yyyymmdd[32], cmd[256];
	char	log_dir[256], log_file[64], subdir[64], suffix_form[32];
    int     cipher;
	time_t	now;
	DIR		*dp;
	struct tm tm;

	now = time(0);

	// 마지막 기록 날짜와 현재 날짜가 같은지 비교한다.
	// 하루 분량의 초단위로 나눠서 비교하면 날짜가 같은지 알 수 있다.
	if (myLogTbl->logInfo[logId].fp != NULL) {
		if (loglib_getDay(now) == loglib_getDay(myLogTbl->logInfo[logId].lastTime)) 
			return 1;
	}

	// full name인 fname을 directory부와 file name부로 나눈다.
	//
	commlib_getLastTokenInString (myLogTbl->logInfo[logId].pro.fname, "/", log_file, log_dir);
	log_dir[strlen(log_dir)-1] = 0;

    // subdir_format으로 subdir부를 추가로 구성한다.
    //
	if (!strcmp(myLogTbl->logInfo[logId].pro.subdir_format, "")) {
        strcpy (subdir, "");
    } else {
        strftime (subdir, sizeof(subdir), myLogTbl->logInfo[logId].pro.subdir_format, localtime_r(&now, &tm));
    }

	// 오늘 날짜 파일이름을 구성한다.
	//
	strftime(yyyymmdd, 32, "%Y%m%d", localtime_r(&now, &tm));
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_LIMIT_SIZE) {
        cipher = (int)log10(myLogTbl->logInfo[logId].pro.num_suffix - 1) + 1; // 자릿수 계산
        sprintf (suffix_form, "%%s/%%s/%%s.%%s.%%0%dd", cipher);
		sprintf (newFile, suffix_form, log_dir, subdir, log_file, yyyymmdd, 0);
	} else {
		sprintf (newFile, "%s/%s/%s.%s", log_dir, subdir, log_file, yyyymmdd);
	}
	sprintf (myLogTbl->logInfo[logId].daily_fname, "%s/%s/%s.%s", log_dir, subdir, log_file, yyyymmdd);

	// 처음 open하는 경우에는 fp==NULL이므로 생성하고 곧바로 return한다.
	//
	if (myLogTbl->logInfo[logId].fp == NULL) {
		strcpy (myLogTbl->logInfo[logId].dir_name, log_dir);
		strcpy (myLogTbl->logInfo[logId].file_name, log_file);
		goto __OPEN_NEW_DAILY_FILE;
	}

	// 날짜가 바뀐경우 열려있는 파일을 닫고 새로운 파일을 연다.
	//
	fprintf (myLogTbl->logInfo[logId].fp, "\n\n----- FILE CLOSE DUE TO DATE CHANGE -----\n\n");
	fflush (myLogTbl->logInfo[logId].fp);
	fclose (myLogTbl->logInfo[logId].fp);


__OPEN_NEW_DAILY_FILE:

	// directory를 먼저 만든다.
	//
	sprintf (dirName, "%s/%s", myLogTbl->logInfo[logId].dir_name, subdir);
	if ((dp = opendir(dirName)) == NULL) {
		if (errno != ENOENT) {
			fprintf(stderr,"[loglib_checkDate] opendir fail[%s]; errno=%d(%s)(%d)\n",dirName,errno,strerror(errno),errno);
			return -1;
		}
		sprintf (cmd, "/bin/mkdir -p %s", dirName);
		system_popen (cmd);
	} else {
		closedir(dp);
	}

	/* open new file */
	if ((myLogTbl->logInfo[logId].fp = fopen(newFile,"a+")) == NULL) {
		fprintf(stderr,"[loglib_checkDate] fopen fail[%s]; errno=%d(%s)\n",newFile,errno,strerror(errno));
		return -1;
	}

	return 1;

} //-- End of loglib_checkDate --//


//------------------------------------------------------------------------------
// LOGLIB_MODE_HOURLY인 경우 시간(Hour)이 바뀌었는지 확인한다.
// - 시간이 바뀐 경우 현재 시간 파일을 연다.
// - directory가 없으면 directory를 먼저 생성한 후 해당 파일을 생성한다.
//------------------------------------------------------------------------------
int loglib_checkTimeHour (int logId)
{
    char	newFile[256], mmddhh[32], cmd[256];
    char	log_dir[256], log_file[64], subdir[64];
    char	dirName[256], suffix_form[32];
    int     cipher;
    time_t	now;
    DIR		*dp;

    now = time(0);

    // 마지막 기록 시간와 현재 시간이 같은지 비교한다.
    // 1시간 분량의 초단위로 나눠서 비교하면 시간 같은지 알 수 있다.
    //
    if (myLogTbl->logInfo[logId].fp != NULL) {
        if ((now / 3600) == (myLogTbl->logInfo[logId].lastTime / 3600)) 
            return 1;
    }

    // full name인 fname을 directory부와 file name부로 나눈다.
    //
    commlib_getLastTokenInString (myLogTbl->logInfo[logId].pro.fname, "/", log_file, log_dir);
    log_dir[strlen(log_dir)-1] = 0;

    // subdir_format으로 subdir부를 추가로 구성한다.
    //
    if (!strcmp(myLogTbl->logInfo[logId].pro.subdir_format, "")) {
        strcpy (subdir, "");
    } else {
        strftime (subdir, sizeof(subdir), myLogTbl->logInfo[logId].pro.subdir_format, localtime(&now));
    }

	// 현재 시간 파일이름을 구성한다.
	//
	strftime (mmddhh, sizeof(mmddhh), "%Y%m%d%H", localtime(&now));
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_LIMIT_SIZE) {
        cipher = (int)log10(myLogTbl->logInfo[logId].pro.num_suffix - 1) + 1; // 자릿수 계산
        sprintf (suffix_form, "%%s/%%s/%%s.%%s.%%0%dd", cipher);
		sprintf (newFile, suffix_form, log_dir, subdir, log_file, mmddhh, 0);
	} else {
		sprintf (newFile, "%s/%s/%s.%s", log_dir, subdir, log_file, mmddhh);
	}
	sprintf (myLogTbl->logInfo[logId].hourly_fname, "%s/%s/%s.%s", log_dir, subdir, log_file, mmddhh);

	// 처음 open하는 경우에는 fp==NULL이므로 생성하고 곧바로 return한다.
	//
	if (myLogTbl->logInfo[logId].fp == NULL) {
		strcpy (myLogTbl->logInfo[logId].dir_name, log_dir);
		strcpy (myLogTbl->logInfo[logId].file_name, log_file);
		goto openNewHourlyFile;
	}

	// 시간이 바뀐경우 열려있는 파일을 닫고 새로운 파일을 연다.
	//
	fprintf (myLogTbl->logInfo[logId].fp, "\n\n----- FILE CLOSE DUE TO DATE-TIME CHANGE -----\n\n");
	fflush (myLogTbl->logInfo[logId].fp);
	fclose (myLogTbl->logInfo[logId].fp);

openNewHourlyFile:

	// directory를 먼저 만든다.
	//
	sprintf (dirName, "%s/%s", myLogTbl->logInfo[logId].dir_name, subdir);
	if ((dp = opendir(dirName)) == NULL) {
		if (errno != ENOENT) {
			fprintf(stderr,"[%s] opendir fail[%s]; errno=%d(%s)\n",__func__,dirName,errno,strerror(errno));
			return -1;
		}
		sprintf (cmd, "/bin/mkdir -p %s", dirName);
		system_popen (cmd);
	} else {
		closedir(dp);
	}

	if ((myLogTbl->logInfo[logId].fp = fopen(newFile,"a+")) == NULL) {
		fprintf(stderr,"[%s] fopen fail[%s]; errno=%d(%s)\n",__func__,newFile,errno,strerror(errno));
		return -1;
	}

	return 1;

} //-- End of loglib_checkTimeHour --//


//------------------------------------------------------------------------------
// LOGLIB_MODE_LIMIT_SIZE인 경우 limit size를 초과했는지 확인한다.
// - 초과된 경우 xxxx.0을 1로, 1을 2로, ... 등 기존 파일을 rename한 후 "xxxx.0"을 다시 open한다.
//------------------------------------------------------------------------------
int loglib_checkLimitSize (int logId)
{
    int     i, cipher;
    size_t  max_size;
    char	file_name[256], newFile[256], oldFile[256], suffix_form[32];
    char	log_dir[256], log_file[64], cmd[256];
    struct stat     fStatus;
    
    // hourly, daily mode인 경우 파일 생성 규칙에 따라 접근해야 하는 로그파일의 이름을 결정한다.
    //
    if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_HOURLY) {
        sprintf (file_name, "%s", myLogTbl->logInfo[logId].hourly_fname);
    } else if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_DAILY) {
        sprintf (file_name, "%s", myLogTbl->logInfo[logId].daily_fname);
    } else {
        strcpy (file_name, myLogTbl->logInfo[logId].pro.fname);
    }

    // MAX FILE SIZE
    max_size = myLogTbl->logInfo[logId].pro.limit_val;

    // 실제 log_file은 뒤에 .0이 붙는다.
    //
    cipher = (int)log10(myLogTbl->logInfo[logId].pro.num_suffix - 1) + 1; // 자릿수 계산
    sprintf (suffix_form, "%%s.%%0%dd", cipher);

    sprintf (newFile, suffix_form, file_name, 0);

    // 파일의 유무를 확인한다.
    //
    if (stat(newFile, &fStatus) < 0) {
        if (errno != ENOENT) {
            fprintf(stderr,"[%s] stat fail[%s]; errno=%d(%s)\n",__func__,newFile,errno,strerror(errno));
            return -1;
        }
        // "xxxx.0" 파일이 존재하지 않으면 생성하고 fp를 저장한후 곧바로 return된다.
        goto openNewLimitSizeFile;
    }

    // 아직 limit를 초과하지 않은 경우
    //
    if (fStatus.st_size < max_size) {
        if (myLogTbl->logInfo[logId].fp == NULL) {
            // 파일을 아직 열지 않은 경우 open하여 fp를 저장한 후 return된다.
            goto openNewLimitSizeFile;
        }
        return 1;
    }

    // limit를 초과한 경우,
    // - 열려있는 파일("xxxx.0")을 close하고 "xxxx.0"을 "xxxx.1"로, 1을 2로, ... 등
    //	기존 파일을 rename한 후 "xxxx.0"을 다시 open한다.
    //

    // close current file
    if (myLogTbl->logInfo[logId].fp != NULL) {
        fprintf (myLogTbl->logInfo[logId].fp, "\n\n----- FILE CLOSE DUE TO SIZE LIMIT -----\n\n");
        fflush (myLogTbl->logInfo[logId].fp);
        fclose (myLogTbl->logInfo[logId].fp);
    }

    // rename files
    //
    for (i = myLogTbl->logInfo[logId].pro.num_suffix - 2; i >= 0; i--) {
        sprintf (oldFile, suffix_form, file_name, i);
        if (stat (oldFile, &fStatus) == 0) {
            sprintf(newFile, suffix_form, file_name, i+1);
            rename (oldFile, newFile);
        }
    }

openNewLimitSizeFile:
    // reopen "xxxx.0"
    sprintf (newFile, suffix_form, file_name, 0);
    if ((myLogTbl->logInfo[logId].fp = fopen(newFile,"a+")) == NULL)
    {
        if (errno != ENOENT) {
            fprintf(stderr,"[%s] fopen fail[%s]; errno=%d(%s)\n",__func__,newFile,errno,strerror(errno));
            return -1;
        }
        // full name인 fname을 directory부와 file name부로 나눈다.
        commlib_getLastTokenInString (file_name, "/", log_file, log_dir);
        log_dir[strlen(log_dir)-1] = 0;
        sprintf (cmd, "/bin/mkdir -p %s", log_dir);
        system_popen (cmd);
        if ((myLogTbl->logInfo[logId].fp = fopen(newFile,"a+")) == NULL) {
            fprintf(stderr,"[%s] fopen fail[%s]; errno=%d(%s)\n",__func__,newFile,errno,strerror(errno));
            return -1;
        }
    }

    return 1;

} //-- End of loglib_checkLimitSize --//

void loglib_getLogFname(int logId, char *fname)
{
   	sprintf(fname, "%s/", myLogTbl->logInfo[logId].dir_name);
	return;
}

/* for debug print */
void __printf(char *fName, int lNum, char*fmt, ...)
{
	char    buff[4096], optBuf[256], tmp[64];
	va_list args;
	struct timeval  curr;

	va_start(args, fmt);
	(void)vsprintf(buff, fmt, args);
	va_end(args);

	gettimeofday (&curr, NULL);
	strcpy (optBuf, "");

	// 현재 시각을 맨 앞에 함께 기록한다.
	strftime(tmp, 32, "%m-%d %T", localtime(&curr.tv_sec));
	sprintf (optBuf, "[%s.%03d] ", tmp, (int)(curr.tv_usec/1000));

	// 소스 파일 이름과 라인 수를 함께 기록한다.
	sprintf(tmp, "[%s:%04d] ", fName, lNum);
	strcat (optBuf, tmp);

	fprintf (stderr, "%s"DRED("DEBUG>>> %s")"\n", optBuf, buff);
}

/* for debugging */
void loglib_printMyLogTbl()
{
	int i, len = 0;
	char buf[4096];

	for(i = 0; i < MAX_APPLOG_NUM; i++) {
		if(myLogTbl->logInfo[i].fp == NULL)
			continue;
		len += sprintf(&buf[len], "[%2d] %10s LVL=%d fp=%p DIR=%s FILE=%s\n",
				i, myLogTbl->appName, myLogTbl->logInfo[i].level,
				myLogTbl->logInfo[i].fp, myLogTbl->logInfo[i].dir_name, myLogTbl->logInfo[i].file_name);
	}
	logPrint(ELI, FL, "\n%s\n", buf);
	fprintf(stderr, "%s\n", buf);
}

/* force log flush */
void loglib_flush(int logId)
{
	if(logId < 0 || logId >= MAX_APPLOG_NUM) {
		return;
	}

// fprintf(stderr, "KHL FP=%d:%p\n", logId, myLogTbl->logInfo[logId].fp);

	if(myLogTbl->logInfo[logId].fp != NULL)
		fflush(myLogTbl->logInfo[logId].fp);
}

int loglib_get_shm_level (int logId)
{
	if (logId < 0 || logId >= MAX_APPLOG_NUM ) {
		fprintf(stderr,"[%s] invalid logId(%d)\n", __func__, logId);
		return -1;
	}

	return (myLogTbl->logInfo[logId].shm_level ? *myLogTbl->logInfo[logId].shm_level : -1);
}

int loglib_set_shm_level (int logId, int level)
{
	if (logId < 0 || logId >= MAX_APPLOG_NUM ) {
		fprintf(stderr,"[%s] invalid logId(%d)\n", __func__, logId);
		return -1;
	}

	if (myLogTbl->logInfo[logId].shm_level)
	{
		*myLogTbl->logInfo[logId].shm_level = level;
		return 0;
	}

	return -1;
}

int
loglib_init_shm_level (void)
{
	if (shm_loglvl)
		return 0;

	size_t shm_size = sizeof (struct log_level_info);
	int ret = commlib_shm_get (SHM_LOGLEVEL, shm_size, (void **)&shm_loglvl);
	if (ret < 0) 
	{	
        fprintf(stderr,"[%s] shmget fail(%s); size=%ld err=%d[%s]\n", 
				__func__, SHM_LOGLEVEL, (unsigned long)shm_size, errno, strerror(errno));
        return -1;
	}

	if (ret == SHM_CREATE)
		memset(shm_loglvl, 0x00, shm_size);

	return 0;
}

int loglib_attach_shm_level (int proc_index, int logId)
{
	if( logId < 0 || logId >= MAX_APPLOG_NUM) 
	{
		fprintf(stderr,"[%s] invalid logId(%d)\n", __func__, logId);
		return -1;
	}

	if (shm_loglvl == NULL)
	{
		if (loglib_init_shm_level () < 0)
			return -1;
	}

	if (proc_index < sizeof (shm_loglvl->logInfo)/ sizeof (shm_loglvl->logInfo[0]))
	{
		myLogTbl->logInfo[logId].shm_level = &shm_loglvl->logInfo[proc_index].logLevel;
		return shm_loglvl->logInfo[proc_index].logLevel;
	}

	return -1;
}


