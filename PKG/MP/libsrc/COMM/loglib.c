#include <commlib.h>
#include <stddef.h>

/* 
 * trcLogId, trcErrLogId, msgLogId, cdrLogId
 * ������ APP BLOCK���� ������ ���� �� �� 
 * loglib.h �� include�Ͽ� ����� �� 
 */
int     trcLogId;       /* debug_log ������ ��� */
int     trcErrLogId;    /* error_log ������ ��� */
int     trcMsgLogId;    /* �����޽��� �α׷� ��� */
int     cdrLogId;       /* cdr_log ������ ��� */

static LogInformationTable	*log_tbl;			/* log table SHM */
static LogInformationTable	gLogTbl;			/* For not registered APPLICATIONS. TOOL, etc... */
#ifdef _SHM_LOGPROP
LogInformationTable			*myLogTbl = NULL;	/* ��Ϻ� log table ���� */
#else
LogInformationTable			*myLogTbl = &gLogTbl;	/* ��Ϻ� log table ���� */
#endif
struct log_level_info		*shm_loglvl = NULL;

static pthread_mutex_t loglibLock = PTHREAD_MUTEX_INITIALIZER;

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
//
// �α׸� ������ �����ϰ�, �ش� ���Ͽ� text�� �α׸� ����ϴ� ����� �����ϴ� library�̴�.
//
// loglib_openLog()�� ȣ���Ҷ� ���ϴ� ������ �̸��� ������ mode�� ������ �� �ִ�.
//
// mode�� ũ�� ������
// 1. text�� �տ� �αױ�� �ð��� �����ϰų� ������ �̸��� ���μ��� �����ϴ� option��
// 2. ������ ���� ��Ģ�� �����ϴ� option���� ����������.
//
// LOGLIB_TIME_STAMP : text �տ� ���� �ð��� [��-��-�� ��:��:��.msec] format���� �߰��Ѵ�.
//                     ex) [2007-06-21 22:41:11.421] this is log message
//
// LOGLIB_FNAME_LNUM : text �տ� lib�� ȣ���� ������ �̸��� ���μ� (__FILE__, __LINE___)�� �߰��Ѵ�.
//                     ex) [test.c:138] this is log message
//
// LOGLIB_MODE_DAILY : ������ ��¥���� ����ϴ� option����
//                     - ������ ������ directory�ο� file�η� ������ directory_name�� sub directory��
//                     - ���糯¥�� �ش��ϴ� directory�� �����ϰ� file_name�� �ش��ϴ� ������ ����Ѵ�.
//                     - ���� ��� �� ��¥�� ����Ǹ� �ڵ����� ���� ������ �ݰ�,
//                       ���ο� ��¥�� �ش��ϴ� directory�� �ٽ� �����ϰ� file_name�� �ش����Ͽ� ����Ѵ�.
// LOGLIB_MODE_HOURLY : ������ �ð��뺰�� ����ϴ� option����
//                     - ������ ������ directory�ο� file�η� ������ directory_name�� sub directory��
//                     - ����ð��� �ش��ϴ� directory�� �����ϰ� file_name.mmddhh�� �ش��ϴ� ������ ����Ѵ�.
//                     - ���� ��� �� �ð��밡 ����Ǹ� �ڵ����� ���� ������ �ݰ�,
//                       ���ο� �ð��뿡 �ش��ϴ� directory�� file�� �ٽ� �����ϰ� �ش����Ͽ� ����Ѵ�.
//
// LOGLIB_MODE_LIMIT_SIZE : ���� �ִ� ũ�⸦ �����ϴ� option����
//                          - ������ ������ ���� ���� 0�� ���̰�, �ִ�ũ�⸦ �ʰ��ϸ� ���������� �ݰ�
//                            ���������� �ٸ��̸����� rename�� �� �ٽ� ������ ���� ����Ѵ�.
//                            ���ϳ��� �ٴ� ���ڴ� 0 ~ max���� �����ϸ� .0 ������ ���� ��ϵǰ� �ִ� �����̴�.
//                          ex) fname = "/tmp/log_dir/log_file"
//                              --> /tmp/log_dir/log_file.0 �� ���Ͽ� ��ϵȴ�.
//                          - �� ������ �ִ� ũ��� ��÷�ڸ� ����� ������ ���� property�� �������� ������
//                            loglib.h�� �ִ� default ������ �����ȴ�.
//
// ��� option�� "|"�� �̿��Ͽ� �ߺ� ������ �� ������
// - ��, LOGLIB_MODE_DAILY�� LOGLIB_MODE_HOURLY�� ���ÿ� ������ �� ����.
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

    // DAILY ���� HOURLY ��带 �ߺ��ؼ� ������ �� ����.
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


    // property ����
    memcpy (&myLogTbl->logInfo[logId].pro, pro, sizeof(LoglibProperty));

    // ��� �� ���� �˻� �� default ����
    if (pro->num_suffix < LOGLIB_NUM_SUFFIX_MIN || pro->num_suffix > LOGLIB_NUM_SUFFIX_MAX)
        myLogTbl->logInfo[logId].pro.num_suffix = LOGLIB_DEFAULT_NUM_SUFFIX;
    if (pro->limit_val < LOGLIB_SIZE_LIMIT_MIN || pro->limit_val > LOGLIB_SIZE_LIMIT_MAX)
        myLogTbl->logInfo[logId].pro.limit_val = LOGLIB_DEFAULT_SIZE_LIMIT;

    // subdir_format �˻�. strftime()�� ȣ���� �����ν� ��ȿ�� �˻�
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


    // LOGLIB_MODE_DAILY�� ���, ���ó�¥�� ����(log_dir/log_file_yyyymmdd) append ����
    //	open�ϰ� myLogTbl->logInfo[logId].fp�� ����Ǿ� return�ȴ�.
    // - LOGLIB_MODE_nDAYS�� ��� n���� ������ �����.
    //
    if (pro->mode & LOGLIB_MODE_DAILY) {
        if (loglib_checkDate(logId) < 0)
            return -1;
        myLogTbl->logInfo[logId].lastTime = time(0);
    }

    // LOGLIB_MODE_HOURLY�� ���, ����ð� �̸��� ����(log_dir/yyyy-mm-dd/log_file.mmddhh) append ����
    //	open�ϰ� myLogTbl->logInfo[logId].fp�� ����Ǿ� return�ȴ�.
    // - directory�� ������ ���� ������ �� ������ �����Ѵ�.
    //
    if (pro->mode & LOGLIB_MODE_HOURLY) {
        if (loglib_checkTimeHour(logId) < 0)
            return -1;
        myLogTbl->logInfo[logId].lastTime = time(0);
    }

    // LOGLIB_MODE_LIMIT_SIZE�� ��� "xxxx.0" ������ ������ Ȯ���ϰ�,
    // - ������ ������ �� myLogTbl->logInfo[logId].fp�� ����Ǿ� return�ǰ�,
    // - ������ limit �ʰ� ���θ� Ȯ���Ͽ� �ʰ����� �ʾ����� append ���� open�� ��
    //	myLogTbl->logInfo[logId].fp�� ����Ǿ� return�ǰ�,
    // - ������ �����ϰ� limit�� �ʰ��� ��� ���������� rename�� �� "xxxx.0"��
    //	�ٽ� �����Ͽ� myLogTbl->logInfo[logId].fp�� �����ϰ� return�ȴ�.
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
		// full name�� fname�� directory�ο� file name�η� ������.
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
		// ���� �����ִ� ���������� ���� �ð��� ���Ͽ� ��¥�� �ٲ������ Ȯ���Ѵ�.
		// - ��¥�� �ٲ������ ���� ��¥ ������ ����.
		// - LOGLIB_MODE_nDAYS�� ��� n���� ������ �����.
		//
		if (loglib_checkDate(logId) < 0)
			goto __FAIL_RETURN;
	}
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_HOURLY)
	{
		// ���� �����ִ� ���������� ���� �ð��� ���Ͽ� �ð�(hour)�� �ٲ������ Ȯ���Ѵ�.
		// - �ð��� �ٲ������ ���� �ð� ������ ����.
		// - ��-��-�� directory ���� �ؿ� ������ �����ǹǷ� directory�� ������ ���� �����.
		//
		if (loglib_checkTimeHour(logId) < 0)
			goto __FAIL_RETURN;
	}
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_LIMIT_SIZE)
	{
		// ���� �����ִ� ������ limit�� �ʰ��ߴ��� Ȯ���Ѵ�.
		// - �ʰ��� ��� ���������� rename�ϰ� �ٽ� ����.
		//
		if (loglib_checkLimitSize(logId) < 0)
			goto __FAIL_RETURN;
	}
	if ( myLogTbl->logInfo[logId].fp == NULL) {
		goto __FAIL_RETURN;
	}

	gettimeofday (&curr, NULL);

	// LOGLIB_TIME_STAMP�̸� ���� �ð��� �� �տ� �Բ� ����Ѵ�.
    //
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_TIME_STAMP) {
		strftime(tmp, 32, "%m-%d %T", localtime(&curr.tv_sec));
		sprintf (optBuf, "[%s.%03d] ", tmp, (int)(curr.tv_usec/1000));
		/*sprintf (optBuf, "[%s.%06d] ", tmp, (int)curr.tv_usec);*/
	}
	// LOGLIB_FNAME_LNUM�̸� �ҽ� ���� �̸��� ���� ���� �Բ� ����Ѵ�.
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


//----------------- ������� ---------------------------------------------------
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
		// ���� �����ִ� ���������� ���� �ð��� ���Ͽ� ��¥�� �ٲ������ Ȯ���Ѵ�.
		// - ��¥�� �ٲ������ ���� ��¥ ������ ����.
		// - LOGLIB_MODE_nDAYS�� ��� n���� ������ �����.
		//
		if (loglib_checkDate(logId) < 0)
			goto __FAIL_RETURN;
	}
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_HOURLY)
	{
		// ���� �����ִ� ���������� ���� �ð��� ���Ͽ� �ð�(hour)�� �ٲ������ Ȯ���Ѵ�.
		// - �ð��� �ٲ������ ���� �ð� ������ ����.
		// - ��-��-�� directory ���� �ؿ� ������ �����ǹǷ� directory�� ������ ���� �����.
		//
		if (loglib_checkTimeHour(logId) < 0)
			goto __FAIL_RETURN;
	}
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_LIMIT_SIZE)
	{
		// ���� �����ִ� ������ limit�� �ʰ��ߴ��� Ȯ���Ѵ�.
		// - �ʰ��� ��� ���������� rename�ϰ� �ٽ� ����.
		//
		if (loglib_checkLimitSize(logId) < 0)
			goto __FAIL_RETURN;
	}
	if ( myLogTbl->logInfo[logId].fp == NULL) {
		goto __FAIL_RETURN;
	}

	gettimeofday (&curr, NULL);
	strcpy (optBuf, "");

	// LOGLIB_TIME_STAMP�̸� ���� �ð��� �� �տ� �Բ� ����Ѵ�.
    //
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_TIME_STAMP) {
		strftime(tmp, 32, "%m-%d %T", localtime(&curr.tv_sec));
		sprintf (optBuf, "[%s.%03d] ", tmp, (int)(curr.tv_usec/1000));
		/*sprintf (optBuf, "[%s.%06d] ", tmp, (int)curr.tv_usec);*/
	}
	// LOGLIB_FNAME_LNUM�̸� �ҽ� ���� �̸��� ���� ���� �Բ� ����Ѵ�.
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
		// ���� �����ִ� ���������� ���� �ð��� ���Ͽ� ��¥�� �ٲ������ Ȯ���Ѵ�.
		// - ��¥�� �ٲ������ ���� ��¥ ������ ����.
		// - LOGLIB_MODE_nDAYS�� ��� n���� ������ �����.
		//
		if (loglib_checkDate(logId) < 0)
			goto __FAIL_RETURN;
	}
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_HOURLY)
	{
		// ���� �����ִ� ���������� ���� �ð��� ���Ͽ� �ð�(hour)�� �ٲ������ Ȯ���Ѵ�.
		// - �ð��� �ٲ������ ���� �ð� ������ ����.
		// - ��-��-�� directory ���� �ؿ� ������ �����ǹǷ� directory�� ������ ���� �����.
		//
		if (loglib_checkTimeHour(logId) < 0)
			goto __FAIL_RETURN;
	}
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_LIMIT_SIZE)
	{
		// ���� �����ִ� ������ limit�� �ʰ��ߴ��� Ȯ���Ѵ�.
		// - �ʰ��� ��� ���������� rename�ϰ� �ٽ� ����.
		//
		if (loglib_checkLimitSize(logId) < 0)
			goto __FAIL_RETURN;
	}
	if ( myLogTbl->logInfo[logId].fp == NULL) {
		goto __FAIL_RETURN;
	}

	gettimeofday (&curr, NULL);
	strcpy (optBuf, "");

	// LOGLIB_TIME_STAMP�̸� ���� �ð��� �� �տ� �Բ� ����Ѵ�.
    //
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_TIME_STAMP) {
		strftime(tmp, 32, "%m-%d %T", localtime(&curr.tv_sec));
		/*sprintf (optBuf, "[%s.%03d] ", tmp, (int)(curr.tv_usec/1000));*/
		sprintf (optBuf, "[%s.%06d] ", tmp, (int)curr.tv_usec);
	}
	// LOGLIB_FNAME_LNUM�̸� �ҽ� ���� �̸��� ���� ���� �Բ� ����Ѵ�.
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
		// ���� �����ִ� ���������� ���� �ð��� ���Ͽ� ��¥�� �ٲ������ Ȯ���Ѵ�.
		// - ��¥�� �ٲ������ ���� ��¥ ������ ����.
		// - LOGLIB_MODE_nDAYS�� ��� n���� ������ �����.
		//
		if (loglib_checkDate(logId) < 0)
			goto __FAIL_RETURN;
	}
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_HOURLY)
	{
		// ���� �����ִ� ���������� ���� �ð��� ���Ͽ� �ð�(hour)�� �ٲ������ Ȯ���Ѵ�.
		// - �ð��� �ٲ������ ���� �ð� ������ ����.
		// - ��-��-�� directory ���� �ؿ� ������ �����ǹǷ� directory�� ������ ���� �����.
		//
		if (loglib_checkTimeHour(logId) < 0)
			goto __FAIL_RETURN;
	}
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_LIMIT_SIZE)
	{
		// ���� �����ִ� ������ limit�� �ʰ��ߴ��� Ȯ���Ѵ�.
		// - �ʰ��� ��� ���������� rename�ϰ� �ٽ� ����.
		//
		if (loglib_checkLimitSize(logId) < 0)
			goto __FAIL_RETURN;
	}
	if ( myLogTbl->logInfo[logId].fp == NULL) {
		goto __FAIL_RETURN;
	}

	gettimeofday (&curr, NULL);
	strcpy (optBuf, "");

	// LOGLIB_TIME_STAMP�̸� ���� �ð��� �� �տ� �Բ� ����Ѵ�.
    //
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_TIME_STAMP) {
		strftime(tmp, 32, "%m-%d %T", localtime(&curr.tv_sec));
		/*sprintf (optBuf, "[%s.%03d] ", tmp, (int)(curr.tv_usec/1000));*/
		sprintf (optBuf, "[%s.%06d] ", tmp, (int)curr.tv_usec);
	}
	// LOGLIB_FNAME_LNUM�̸� �ҽ� ���� �̸��� ���� ���� �Բ� ����Ѵ�.
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

/* __lprintf�� ������. ��, color print ������ */
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
		// ���� �����ִ� ���������� ���� �ð��� ���Ͽ� ��¥�� �ٲ������ Ȯ���Ѵ�.
		// - ��¥�� �ٲ������ ���� ��¥ ������ ����.
		// - LOGLIB_MODE_nDAYS�� ��� n���� ������ �����.
		//
		if (loglib_checkDate(logId) < 0)
			goto __FAIL_RETURN;
	}
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_HOURLY)
	{
		// ���� �����ִ� ���������� ���� �ð��� ���Ͽ� �ð�(hour)�� �ٲ������ Ȯ���Ѵ�.
		// - �ð��� �ٲ������ ���� �ð� ������ ����.
		// - ��-��-�� directory ���� �ؿ� ������ �����ǹǷ� directory�� ������ ���� �����.
		//
		if (loglib_checkTimeHour(logId) < 0)
			goto __FAIL_RETURN;
	}
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_LIMIT_SIZE)
	{
		// ���� �����ִ� ������ limit�� �ʰ��ߴ��� Ȯ���Ѵ�.
		// - �ʰ��� ��� ���������� rename�ϰ� �ٽ� ����.
		//
		if (loglib_checkLimitSize(logId) < 0)
			goto __FAIL_RETURN;
	}
	if ( myLogTbl->logInfo[logId].fp == NULL) {
		goto __FAIL_RETURN;
	}

	gettimeofday (&curr, NULL);
	strcpy (optBuf, "");

	// LOGLIB_TIME_STAMP�̸� ���� �ð��� �� �տ� �Բ� ����Ѵ�.
    //
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_TIME_STAMP) {
		strftime(tmp, 32, "%m-%d %T", localtime(&curr.tv_sec));
		/*sprintf (optBuf, "[%s.%03d] ", tmp, (int)(curr.tv_usec/1000));*/
		sprintf (optBuf, "[%s.%06d] ", tmp, (int)curr.tv_usec);
	}
	// LOGLIB_FNAME_LNUM�̸� �ҽ� ���� �̸��� ���� ���� �Բ� ����Ѵ�.
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
// yyyymmdd �� int �� return
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
// LOGLIB_MODE_DAILY�� ��� ��¥�� �ٲ������ Ȯ���Ѵ�.
// - ��¥�� �ٲ� ��� ���� ��¥ ������ ����.
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

	// ������ ��� ��¥�� ���� ��¥�� ������ ���Ѵ�.
	// �Ϸ� �з��� �ʴ����� ������ ���ϸ� ��¥�� ������ �� �� �ִ�.
	if (myLogTbl->logInfo[logId].fp != NULL) {
		if (loglib_getDay(now) == loglib_getDay(myLogTbl->logInfo[logId].lastTime)) 
			return 1;
	}

	// full name�� fname�� directory�ο� file name�η� ������.
	//
	commlib_getLastTokenInString (myLogTbl->logInfo[logId].pro.fname, "/", log_file, log_dir);
	log_dir[strlen(log_dir)-1] = 0;

    // subdir_format���� subdir�θ� �߰��� �����Ѵ�.
    //
	if (!strcmp(myLogTbl->logInfo[logId].pro.subdir_format, "")) {
        strcpy (subdir, "");
    } else {
        strftime (subdir, sizeof(subdir), myLogTbl->logInfo[logId].pro.subdir_format, localtime_r(&now, &tm));
    }

	// ���� ��¥ �����̸��� �����Ѵ�.
	//
	strftime(yyyymmdd, 32, "%Y%m%d", localtime_r(&now, &tm));
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_LIMIT_SIZE) {
        cipher = (int)log10(myLogTbl->logInfo[logId].pro.num_suffix - 1) + 1; // �ڸ��� ���
        sprintf (suffix_form, "%%s/%%s/%%s.%%s.%%0%dd", cipher);
		sprintf (newFile, suffix_form, log_dir, subdir, log_file, yyyymmdd, 0);
	} else {
		sprintf (newFile, "%s/%s/%s.%s", log_dir, subdir, log_file, yyyymmdd);
	}
	sprintf (myLogTbl->logInfo[logId].daily_fname, "%s/%s/%s.%s", log_dir, subdir, log_file, yyyymmdd);

	// ó�� open�ϴ� ��쿡�� fp==NULL�̹Ƿ� �����ϰ� ��ٷ� return�Ѵ�.
	//
	if (myLogTbl->logInfo[logId].fp == NULL) {
		strcpy (myLogTbl->logInfo[logId].dir_name, log_dir);
		strcpy (myLogTbl->logInfo[logId].file_name, log_file);
		goto __OPEN_NEW_DAILY_FILE;
	}

	// ��¥�� �ٲ��� �����ִ� ������ �ݰ� ���ο� ������ ����.
	//
	fprintf (myLogTbl->logInfo[logId].fp, "\n\n----- FILE CLOSE DUE TO DATE CHANGE -----\n\n");
	fflush (myLogTbl->logInfo[logId].fp);
	fclose (myLogTbl->logInfo[logId].fp);


__OPEN_NEW_DAILY_FILE:

	// directory�� ���� �����.
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
// LOGLIB_MODE_HOURLY�� ��� �ð�(Hour)�� �ٲ������ Ȯ���Ѵ�.
// - �ð��� �ٲ� ��� ���� �ð� ������ ����.
// - directory�� ������ directory�� ���� ������ �� �ش� ������ �����Ѵ�.
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

    // ������ ��� �ð��� ���� �ð��� ������ ���Ѵ�.
    // 1�ð� �з��� �ʴ����� ������ ���ϸ� �ð� ������ �� �� �ִ�.
    //
    if (myLogTbl->logInfo[logId].fp != NULL) {
        if ((now / 3600) == (myLogTbl->logInfo[logId].lastTime / 3600)) 
            return 1;
    }

    // full name�� fname�� directory�ο� file name�η� ������.
    //
    commlib_getLastTokenInString (myLogTbl->logInfo[logId].pro.fname, "/", log_file, log_dir);
    log_dir[strlen(log_dir)-1] = 0;

    // subdir_format���� subdir�θ� �߰��� �����Ѵ�.
    //
    if (!strcmp(myLogTbl->logInfo[logId].pro.subdir_format, "")) {
        strcpy (subdir, "");
    } else {
        strftime (subdir, sizeof(subdir), myLogTbl->logInfo[logId].pro.subdir_format, localtime(&now));
    }

	// ���� �ð� �����̸��� �����Ѵ�.
	//
	strftime (mmddhh, sizeof(mmddhh), "%Y%m%d%H", localtime(&now));
	if (myLogTbl->logInfo[logId].pro.mode & LOGLIB_MODE_LIMIT_SIZE) {
        cipher = (int)log10(myLogTbl->logInfo[logId].pro.num_suffix - 1) + 1; // �ڸ��� ���
        sprintf (suffix_form, "%%s/%%s/%%s.%%s.%%0%dd", cipher);
		sprintf (newFile, suffix_form, log_dir, subdir, log_file, mmddhh, 0);
	} else {
		sprintf (newFile, "%s/%s/%s.%s", log_dir, subdir, log_file, mmddhh);
	}
	sprintf (myLogTbl->logInfo[logId].hourly_fname, "%s/%s/%s.%s", log_dir, subdir, log_file, mmddhh);

	// ó�� open�ϴ� ��쿡�� fp==NULL�̹Ƿ� �����ϰ� ��ٷ� return�Ѵ�.
	//
	if (myLogTbl->logInfo[logId].fp == NULL) {
		strcpy (myLogTbl->logInfo[logId].dir_name, log_dir);
		strcpy (myLogTbl->logInfo[logId].file_name, log_file);
		goto openNewHourlyFile;
	}

	// �ð��� �ٲ��� �����ִ� ������ �ݰ� ���ο� ������ ����.
	//
	fprintf (myLogTbl->logInfo[logId].fp, "\n\n----- FILE CLOSE DUE TO DATE-TIME CHANGE -----\n\n");
	fflush (myLogTbl->logInfo[logId].fp);
	fclose (myLogTbl->logInfo[logId].fp);

openNewHourlyFile:

	// directory�� ���� �����.
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
// LOGLIB_MODE_LIMIT_SIZE�� ��� limit size�� �ʰ��ߴ��� Ȯ���Ѵ�.
// - �ʰ��� ��� xxxx.0�� 1��, 1�� 2��, ... �� ���� ������ rename�� �� "xxxx.0"�� �ٽ� open�Ѵ�.
//------------------------------------------------------------------------------
int loglib_checkLimitSize (int logId)
{
    int     i, cipher;
    size_t  max_size;
    char	file_name[256], newFile[256], oldFile[256], suffix_form[32];
    char	log_dir[256], log_file[64], cmd[256];
    struct stat     fStatus;
    
    // hourly, daily mode�� ��� ���� ���� ��Ģ�� ���� �����ؾ� �ϴ� �α������� �̸��� �����Ѵ�.
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

    // ���� log_file�� �ڿ� .0�� �ٴ´�.
    //
    cipher = (int)log10(myLogTbl->logInfo[logId].pro.num_suffix - 1) + 1; // �ڸ��� ���
    sprintf (suffix_form, "%%s.%%0%dd", cipher);

    sprintf (newFile, suffix_form, file_name, 0);

    // ������ ������ Ȯ���Ѵ�.
    //
    if (stat(newFile, &fStatus) < 0) {
        if (errno != ENOENT) {
            fprintf(stderr,"[%s] stat fail[%s]; errno=%d(%s)\n",__func__,newFile,errno,strerror(errno));
            return -1;
        }
        // "xxxx.0" ������ �������� ������ �����ϰ� fp�� �������� ��ٷ� return�ȴ�.
        goto openNewLimitSizeFile;
    }

    // ���� limit�� �ʰ����� ���� ���
    //
    if (fStatus.st_size < max_size) {
        if (myLogTbl->logInfo[logId].fp == NULL) {
            // ������ ���� ���� ���� ��� open�Ͽ� fp�� ������ �� return�ȴ�.
            goto openNewLimitSizeFile;
        }
        return 1;
    }

    // limit�� �ʰ��� ���,
    // - �����ִ� ����("xxxx.0")�� close�ϰ� "xxxx.0"�� "xxxx.1"��, 1�� 2��, ... ��
    //	���� ������ rename�� �� "xxxx.0"�� �ٽ� open�Ѵ�.
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
        // full name�� fname�� directory�ο� file name�η� ������.
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

	// ���� �ð��� �� �տ� �Բ� ����Ѵ�.
	strftime(tmp, 32, "%m-%d %T", localtime(&curr.tv_sec));
	sprintf (optBuf, "[%s.%03d] ", tmp, (int)(curr.tv_usec/1000));

	// �ҽ� ���� �̸��� ���� ���� �Բ� ����Ѵ�.
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


