#include <signal.h>
#include <time.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/wait.h> 

#include <comm_util.h>
#include <comm_msgtypes.h>
#include <typedef.h>


#define MagicChar(n)    (((n) >= 10) ? 55 : 48)     /* 55('A' - 10), 48('0') */
#define MagicNumber(c)  (((c) >= 65) ? 55 : 48)     /* 55('A' - 10), 48('0') */
#define Two2Int(a)  (((a)[0] - '0') * 10 + (a)[1] - '0')

//struct tm *localtime_r(const time_t *timer, struct tm *result);

/*------------------------------------------------------------------------------
  ------------------------------------------------------------------------------*/
void commlib_convByteOrd (char *p, int siz)
{
	char    buff[80];
	int             i;

	for (i=0; i<siz; i++)
		buff[siz-i-1] = p[i];
	memcpy(p,buff,siz);
} /** End of commlib_convByteOrd **/



/*------------------------------------------------------------------------------
  ------------------------------------------------------------------------------*/
void commlib_microSleep (int usec)
{
	struct timeval  sleepTmr;

	sleepTmr.tv_sec  = usec / 1000000;
	sleepTmr.tv_usec = usec % 1000000;
	select(0,0,0,0,&sleepTmr);
	return;
} /** End of commlib_microSleep **/


/*------------------------------------------------------------------------------
  ------------------------------------------------------------------------------*/
void commlib_setupSignals (int *notMaskSig)
{
	int             i, flag, *ptr;

	signal(SIGINT,  commlib_quitSignal);
	signal(SIGTERM, commlib_quitSignal);

	signal(SIGHUP,   commlib_ignoreSignal);
	signal(SIGPIPE,  commlib_ignoreSignal);
	signal(SIGALRM,  commlib_ignoreSignal);

#ifdef TRU64
	for (i=18; i<=SIGMAX; i++)
#else
//	for (i=18; i<=MAXSIG; i++)
	for (i=18; i<=SIGRTMAX; i++)
#endif
	{
		// catch하지 말아야 할 놈으로 지정되었는지 확인한다.
		for (ptr = notMaskSig, flag=0; (ptr != NULL && *ptr != 0); ptr++) {
			if (*ptr == i) {
				flag = 1;
			}
		}
		if (flag) continue;

		signal(i, commlib_ignoreSignal);
	}

	return;

} /** End of commlib_setupSignals **/

#if 0
void call_setupSignals (int *notMaskSig)
{
	int             i, flag, *ptr;

	signal(SIGINT,  commlib_quitSignal);
	signal(SIGTERM, commlib_quitSignal);

	signal(SIGHUP,   commlib_ignoreSignal);
	signal(SIGPIPE,  commlib_ignoreSignal);

#ifdef TRU64
	for (i=18; i<=SIGMAX; i++)
#else
	//for (i=18; i<=MAXSIG; i++)
	for (i=18; i<=SIGRTMAX; i++)
#endif
			{
			// catch하지 말아야 할 놈으로 지정되었는지 확인한다.
			for (ptr = notMaskSig, flag=0; (ptr != NULL && *ptr != 0); ptr++) {
				if (*ptr == i) {
					flag = 1;
				}
			}
			if (flag) continue;

			signal(i, commlib_ignoreSignal);
		}

	return;

} /** End of commlib_setupSignals **/
#endif



/*------------------------------------------------------------------------------
  ------------------------------------------------------------------------------*/
void commlib_quitSignal (int signo)
{
	char    buff[256];
	fprintf(stderr,  ">>> terminated by user request. signo=%d\n", signo);
	exit(1);
} /** End of commlib_quitSignal **/



/*------------------------------------------------------------------------------
  ------------------------------------------------------------------------------*/
void commlib_ignoreSignal (int signo)
{
	int		stat;

    if (signo == SIGCHLD)
        wait(&stat);
	signal (signo, commlib_ignoreSignal);
	return;
} /** End of commlib_quitSignal **/




/*------------------------------------------------------------------------------
  ------------------------------------------------------------------------------*/
char commlib_timeStamp[32];
char *commlib_printTime (void)
{
	time_t  now;
	struct tm       *pLocalTime;

	now = time(0);
	if ((pLocalTime = (struct tm*)localtime((time_t*)&now)) == NULL) {
		strcpy(commlib_timeStamp,"");
	} else {
		strftime(commlib_timeStamp, 32, "%T", pLocalTime);
	}
	return ((char*)commlib_timeStamp);
} /** End of commlib_printTime **/



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
char *commlib_printTStamp (void)
{
	time_t          now;
	struct tm       *pLocalTime;

	now = time(0);

	if ((pLocalTime = (struct tm*)localtime((time_t*)&now)) == NULL) {
		strcpy (commlib_timeStamp,"");
	} else {
		strftime (commlib_timeStamp, 32, "%Y-%m-%d %T %a", pLocalTime);
		commlib_timeStamp[21] = toupper(commlib_timeStamp[21]);
		commlib_timeStamp[22] = toupper(commlib_timeStamp[22]);
	}

	return ((char*)commlib_timeStamp);

} //----- End of commlib_printTStamp -----//

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void commlib_printTStamp2 (char *buf)
{
	time_t          now;
	struct tm       *pLocalTime;

	now = time(0);

	if ((pLocalTime = (struct tm*)localtime((time_t*)&now)) == NULL) {
		strcpy (buf,"");
	} else {
		strftime (buf, 32, "%Y-%m-%d %T %a", pLocalTime);
		buf[21] = toupper(buf[21]);
		buf[22] = toupper(buf[22]);
	}

	return;
} //----- End of commlib_printTStamp -----//

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
char *commlib_printUsec (void)
{
	struct timeval  now; 
    struct tm   	*pLocalTime; 
    char    		tmp[32]; 
 
    gettimeofday (&now, NULL); 
    if ((pLocalTime = localtime(&now.tv_sec)) == NULL) { 
        strcpy(commlib_timeStamp,""); 
    } else { 
        strftime(tmp, 32, "%T", pLocalTime); 
        sprintf (commlib_timeStamp, "%s.%03d", tmp, (int)(now.tv_usec/1000)); 
        //sprintf (timestr, "%s.%06d", tmp, (int)(now.tv_usec)); 
    } 

	return ((char*)commlib_timeStamp);

} //----- End of commlib_printUsec -----//

/* thread-safe */
int commlib_usecStr(char *buf)
{   
    struct timeval  now; 
    struct tm       *pLocalTime; 
    char            tmp[32];
    int             len = 0;
                    
    gettimeofday (&now, NULL);
    if ((pLocalTime = localtime(&now.tv_sec)) == NULL) {
        len += sprintf(buf," "); 
    } else {
        strftime(tmp, 32, "%T", pLocalTime);
        len += sprintf (buf, "%s.%03d", tmp, (int)(now.tv_usec/1000));
    }   
        
    return len;
}   

     

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
char commlib_microTimeStamp[32];
char *commlib_printMicrosec (void)
{
	struct timeval  now; 
    struct tm   	*pLocalTime; 
    char    		tmp[32];
 
    gettimeofday (&now, NULL); 
    if ((pLocalTime = localtime(&now.tv_sec)) == NULL) { 
        strcpy(commlib_microTimeStamp,""); 
    } else { 
        strftime(tmp, 32, "%T", pLocalTime); 
        sprintf (commlib_microTimeStamp, "%s.%06d", tmp, (int)(now.tv_usec)); 
    } 

	return commlib_microTimeStamp;

} //----- End of commlib_printMicrosec -----//


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
char *commlib_printTStampUsec (void)
{
	struct timeval  now; 
    struct tm   	*pLocalTime; 
    char    		tmp[32]; 
 
    gettimeofday (&now, NULL); 
    if ((pLocalTime = localtime(&now.tv_sec)) == NULL) { 
        strcpy(commlib_timeStamp,""); 
    } else { 
		strftime (tmp, 32, "%Y-%m-%d %T", pLocalTime);
        sprintf (commlib_timeStamp, "%s.%03d", tmp, (int)(now.tv_usec/1000)); 
    } 

	return ((char*)commlib_timeStamp);

} //----- End of commlib_printUsec -----//


#if 1	/* jhpark */
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
char *commlib_printTStamp_configFile (void)
{
    struct timeval  now;
    struct tm       *pLocalTime;
    
    char tmp[32];
    
    gettimeofday (&now, NULL);
    
    if ((pLocalTime = (struct tm*)localtime((time_t*)&now.tv_sec)) == NULL) {
        strcpy (commlib_timeStamp,"");
    } else {
        strftime (tmp, 32, "%Y%m%d-%H%M%S", pLocalTime);
        sprintf (commlib_timeStamp, "%s", tmp);
    }

    return ((char*)commlib_timeStamp);

} // ----- commlib_printTStamp_configFile ----- //
#endif

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
char *commlib_printDateTime (time_t when)
{
	struct tm       pLocalTime;

	if (when == 0) {
		strcpy (commlib_timeStamp,"-");
		return ((char*)commlib_timeStamp);
	}
	if ( localtime_r((time_t*)&when, &pLocalTime) == (struct tm *) NULL) {
		strcpy (commlib_timeStamp,"");
	} else {
		strftime (commlib_timeStamp, 32, "%Y-%m-%d %T", &pLocalTime);
	}

	return ((char*)commlib_timeStamp);

} //----- End of commlib_printDateTime -----//


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
char *commlib_printDateHM (time_t when)
{
	struct tm       pLocalTime;

	if ( localtime_r((time_t*)&when, &pLocalTime) == (struct tm *) NULL) {
		strcpy (commlib_timeStamp,"");
	} else {
		strftime (commlib_timeStamp, 32, "%Y-%m-%d %H:%M", &pLocalTime);
	}

	return ((char*)commlib_timeStamp);

} //----- End of commlib_printDateTime -----//

char *commlib_printDateHM2 ()
{
	struct tm       pLocalTime;
	time_t when	= time(0);

	if ( localtime_r((time_t*)&when, &pLocalTime) == (struct tm *) NULL) {
		strcpy (commlib_timeStamp,"");
	} else {
		strftime (commlib_timeStamp, 32, "%Y%m%d%H%M", &pLocalTime);
	}

	return ((char*)commlib_timeStamp);

} //----- End of commlib_printDateHM2 -----//


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int commlib_dateTimeStr (time_t when, char *tbuf)
{
	struct tm   pLocalTime;
	int		len = 0;

	if(when == 0) {
		len = sprintf(tbuf, "0");
		return len;
	}

	if ( localtime_r((time_t*)&when, &pLocalTime) == (struct tm *) NULL) {
		strcpy (tbuf,"");
	} else {
		len = strftime (tbuf, 32, "%Y%m%d%H%M%S", &pLocalTime);
	}

	return len;
}

int commlib_dateTimeStr_z(time_t when, char *tbuf)
{
	struct tm   pLocalTime;
	int		len = 0;

	if(when == 0) {
		tbuf[0] = 0;
		return 0;
	}

	if ( localtime_r((time_t*)&when, &pLocalTime) == (struct tm *) NULL) {
		strcpy (tbuf,"");
	} else {
		len = strftime (tbuf, 32, "%Y%m%d%H%M%S", &pLocalTime);
	}

	return len;
}

int commlib_dateTimeStr2 (time_t when, char *tbuf)
{
	struct tm   pLocalTime;
	int		len = 0;

	if ( localtime_r((time_t*)&when, &pLocalTime) == (struct tm *) NULL) {
		strcpy (tbuf,"");
	} else {
		len = strftime (tbuf, 32, "%Y-%m-%d %H:%M:%S", &pLocalTime);
	}

	return len;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
char *commlib_timeStr (time_t when)
{
	struct tm       pLocalTime;

	if ( localtime_r((time_t*)&when, &pLocalTime) == (struct tm *) NULL) {
		strcpy (commlib_timeStamp,"");
	} else {
		strftime (commlib_timeStamp, 32, "%T", &pLocalTime);
	}

	return ((char*)commlib_timeStamp);

} //----- End of commlib_printDateTime -----//

/* thread-safe */
int commlib_dateStr (time_t when, char *buf)
{
	struct tm       pLocalTime;
	int				len;

	if ( localtime_r((time_t*)&when, &pLocalTime) == (struct tm *) NULL) {
		len = sprintf (buf," ");
	} else {
		len = strftime (buf, 32, "%Y%m%d", &pLocalTime);
	}

	return len;

} //----- End of commlib_dateStr -----//

int commlib_dateStr2 (time_t when, char *buf)
{
	struct tm       pLocalTime;
	int				len;

	if ( localtime_r((time_t*)&when, &pLocalTime) == (struct tm *) NULL) {
		len = sprintf (buf," ");
	} else {
		len = strftime (buf, 32, "%Y-%m-%d", &pLocalTime);
	}

	return len;

} //----- End of commlib_dateStr -----//

int commlib_getTimeStr(time_t when, char* format, char* buf, size_t buf_size)
{
	struct tm       pLocalTime;
	int				len;

	if ( localtime_r((time_t*)&when, &pLocalTime) == (struct tm *) NULL) {
		buf[0] = 0;
		return 0;
	}

	len = strftime (buf, buf_size, format, &pLocalTime);
	return len;
}

/* str: YYYYMM, YYYYMMDD, YYYYMMDDHH, YYYYMMDDHHMM, YYYYMMDDHHMMSS */
bool commlib_cvtStr2Time_14(char *str, time_t *tt)
{
	time_t clk;
	struct tm time_str;

	time_str.tm_isdst = -1;
	time_str.tm_sec = Two2Int((&str[12]));
	time_str.tm_min = Two2Int((&str[10]));
	time_str.tm_hour = Two2Int((&str[8]));
	time_str.tm_mday = Two2Int((&str[6]));
	time_str.tm_mon = Two2Int((&str[4])) - 1;
	time_str.tm_year = (Two2Int(str) - 19) * 100 + Two2Int((&str[2]));

	if ((clk = mktime(&time_str)) == (time_t)-1)
		return false;

	*tt = clk;
	return true;
}


int commlib_nullcmp (unsigned char *src, int len)
{
    int i;

    for (i=0; i<len; i++) {
        if ( src[i] != 0x00 ) return 1;
    }
    return 0;
}

//------------------------------------------------------------------------------
// 현재 시각을 double형으로 계산하여 return한다.
//------------------------------------------------------------------------------
double commlib_getCurrTime_double (void)
{
	struct timeval  now;
	double          tval;

	gettimeofday (&now, NULL);

	tval = (double)now.tv_sec + (double)now.tv_usec/1000000;

	return tval;

} //----- End of commlib_getCurrTime_double -----//

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int commlib_getMachineId_fromString (char *name)
{
	int	id=0;

	if (isupper((int)name[strlen(name)-1])) {
		id = name[strlen(name)-1] - 'A';
	} else if (islower((int)name[strlen(name)-1])) {
		id = name[strlen(name)-1] - 'a';
	} else if (isdigit((int)name[strlen(name)-1])) {
		id = name[strlen(name)-1] - '1';
#if 1
	} else {
		fprintf(stderr,"[commlib_getMachineId] Unexpected Name[%s]\n", name);
		fprintf(stderr,"    ex) ...A | ...B | ...a | ...b | ...1 | ...2 \n");
		return -1;
#endif
   	}

   	return id;

} //----- End of commlib_getMachineId_fromString -----//


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int commlib_getOtherSideMachineName (char *own, char *other)
{
	int		my_id;

	my_id = commlib_getMachineId_fromString (own);
	strcpy (other, own);
	switch (my_id) {
		case 0: other[strlen(other)-1] = (char)(other[strlen(other)-1] + 1); return 1;
		case 1: other[strlen(other)-1] = (char)(other[strlen(other)-1] - 1); return 1;
	}

	return my_id;

} //----- End of commlib_getMachineId_fromString -----//



//------------------------------------------------------------------------------
// src string에서 지정된 delimiter에 의해 구분되는 token 중에 맨 마지막 token을 찾아
//   lastToken에 넣고, 나머지 앞쪽 전체 string을 prefixString에 넣는다.
//------------------------------------------------------------------------------
int commlib_getLastTokenInString (char *src, char *delimiter, char *lastToken, char *prefixString)
{
	int		i, j, not_include = 1;
	char	tmp_str[4096];
	char    *ptr, *next, *last, *start;

	// delimiter가 전혀 포함되어 있지 않은지 먼저 확인한다.
	// - delimiter는 character 단위로 한 문자씩 의미가 있으므로,
	//   strstr() 등으로 포함여부를 확인할 수 없다.
	//
	for (i=0; i<(int)strlen(delimiter); i++) {
		for (j=0; j<(int)strlen(src); j++) {
			if (delimiter[i] == src[j]) break;
		}
		if (j < (int)strlen(src)) {
			not_include = 0;
			break;
		}
	}
	if (not_include) {
		lastToken = prefixString = NULL;
		return -1;
	}

	// 원본 string pointer를 손상시키지 않기 위해 임시 버퍼에 넣어 놓고 작업한다.
	//
	strcpy (tmp_str, src);

	ptr = start = tmp_str;

	while ((ptr = strtok_r (ptr, delimiter, &next)) != NULL) {
		last = ptr;
		ptr = next;
	}

	strcpy (lastToken, last);
	strncpy (prefixString, src, (size_t)(last - start));
	prefixString[last - start] = 0;

	return 1;

} //----- End of commlib_getLastTokenInString -----//



//------------------------------------------------------------------------------
// src string에서 Quotation Mark로 묶인 token을 분리하여 token array에 넣고 token갯수를 리턴한다.
//------------------------------------------------------------------------------
int commlib_tokenizeQuoteMark (char *src, char tokens[16][CONFLIB_MAX_TOKEN_LEN])
{
	int		i, j, cnt=0, start, end, len;

	for (i=cnt=0; i<(int)strlen(src); i++) {
		if (src[i] != '"')
			continue;
		start = i+1; // token 시작 위치
		for (j=i+1; j<(int)strlen(src); j++) {
			if (src[j] != '"')
				continue;
			end = j; // token 끝 위치
			len = end - start;
			strncpy (tokens[cnt], &src[start], (size_t)len);
			tokens[cnt][len] = 0;
			cnt++; // token 갯수 증가
			if (cnt >= 16) return cnt;
			break;
		}
		i = j; // 다음 시작 위치 조정
	}

	return cnt;

} //----- End of commlib_tokenizeQuoteMark -----//


static char tokens_quote[16][CONFLIB_MAX_TOKEN_LEN];
//------------------------------------------------------------------------------
// src string에서 Quotation Mark로 묶인 token을 분리하여 N번째 token을 리턴한다.
//------------------------------------------------------------------------------
char *commlib_getNthTokenQuoteMark (char *src, int n)
{
	int		cnt;

	if (n < 1 || n > 16) {
		fprintf(stderr,"[commlib_getNthTokenQuoteMark] Unexpected Nth[%d]; range(1~16) \n", n);
		return NULL;
	}
	memset (tokens_quote, 0, sizeof(tokens_quote));

	cnt = commlib_tokenizeQuoteMark (src, tokens_quote);
	if (cnt < n) {
		fprintf(stderr,"[commlib_getNthTokenQuoteMark] not enough tokens [%s]; cnt=%d, n=%d \n", src, cnt, n);
		return NULL;
	}

	return (tokens_quote[n-1]);

} //----- End of commlib_getNthTokenQuoteMark -----//



/**
 Home Zone Table 내에 update date 자료형 (varchar14)를 time_t로 바꾸는 함수 
*/
int getTime_tFromDateStr(time_t *destTime, char *srcStr)
{
    struct tm tmpTm;
    char    tmpStr[14+1];
    char    *pnt = tmpStr; 
    memcpy(tmpStr, srcStr, 14); 
    pnt+=14; *pnt='\0';

    pnt-=2; tmpTm.tm_sec = atoi(pnt); *pnt='\0'; 
    pnt-=2; tmpTm.tm_min = atoi(pnt); *pnt='\0'; 
    pnt-=2; tmpTm.tm_hour = atoi(pnt); *pnt='\0'; 
    pnt-=2; tmpTm.tm_mday = atoi(pnt); *pnt='\0'; 
    pnt-=2; tmpTm.tm_mon = atoi(pnt) - 1; *pnt='\0'; 
    pnt-=4; tmpTm.tm_year = atoi(pnt)-1900; 
    
    *destTime = mktime(&tmpTm);

    return 0; 
}

/**
 Home Zone Table 내에 time_t를 char로 바꾸는 함수 
*/
int getTime_charFromTimeT(char *destStr, time_t *srcTime )
{
    char    tmpStr[14+1];
    struct  tm tmpTm;

    memcpy(&tmpTm, localtime(srcTime), sizeof(tmpTm));
    
    sprintf(tmpStr, "%4d%2d%2d%2d%2d%2d"
        ,tmpTm.tm_year + 1900, tmpTm.tm_mon + 1, tmpTm.tm_mday, tmpTm.tm_hour, tmpTm.tm_min, tmpTm.tm_sec);

    memcpy(destStr, tmpStr, sizeof(tmpStr));
   
    return 0; 
}

/*------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------*/
//int commlib_mmcHdlrVector_qsortCmp (const void *a, const void *b)
//{
 //   return (strcasecmp (((MmcHdlrVector*)a)->cmdName, ((MmcHdlrVector*)b)->cmdName));
//} /*----- End of commlib_mmcHdlrVector_qsortCmp -----*/

/*------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------*/
//int commlib_mmcHdlrVector_bsrchCmp (const void *a, const void *b)
//{
//	    return (strcasecmp ((char*)a, ((MmcHdlrVector*)b)->cmdName));
//} /*----- End of commlib_mmcHdlrVector_bsrchCmp -----*/


int commlib_mktime(const char *strTime)
{
    struct tm stm;

    if( strlen(strTime)<14 ) return 0;
    
    stm.tm_isdst = -1;
    stm.tm_sec  = Two2Int(&strTime[12]);
    stm.tm_min  = Two2Int(&strTime[10]);
    stm.tm_hour = Two2Int(&strTime[8]);
    stm.tm_mday = Two2Int(&strTime[6]);
    stm.tm_mon  = Two2Int(&strTime[4])-1;
    stm.tm_year = (Two2Int(strTime)-19)*100 + Two2Int(&strTime[2]);

    return (int)mktime(&stm);
}

#define U_MASK 0x0F
#define MagicChar(n)    (((n) >= 10) ? 55 : 48)     /* 55('A' - 10), 48('0') */

/* hexa 를 char 로 */
void Hexa2char(unsigned char *h, char *c, int len)
{
    register long i;
    register char tch;

    for(i=0;i<len;i++) {
        tch = (char)(U_MASK & h[i]);
        c[i*2] = (char)(tch + MagicChar(tch));
        tch = (char)(h[i] >> 4);
        c[i*2+1] = (char)(tch + MagicChar(tch));
    }
}

/* 1234 -> 0x21, 0x43 */
/* c:src, h:dest */
/* len = src의 길이 */
int Char2Hexa(char *c, unsigned char *h, int len)
{

    int i;
    unsigned char tch;


    for(i=0;i < len/2;i++, h++) {
        *h = (unsigned char)(toupper(*c) - MagicNumber(toupper(*c)));  c++;
        tch = (unsigned char)(toupper(*c) - MagicNumber(toupper(*c))); c++;
        *h = (unsigned char)(*h | tch << 4);
    }
    if(len%2)
        *h = (unsigned char)(toupper(*c) - MagicNumber(toupper(*c)));

    return(0);
}


#define U_MASK 0x0F
void Hexa2char2(unsigned char *h, char *c, int len)
{
    register long i;
    register char tch;

    for(i=0;i<len;i++) {
        tch = (char)(h[i] >> 4);
        c[i*2] = (char)(tch + MagicChar(tch));
        tch = (char)(U_MASK & h[i]);
        c[i*2+1] = (char)(tch + MagicChar(tch));
    }
}

/* 1234 -> 0x12,0x34 */
int Char2Hexa2(char *c, unsigned char *h, int len)
{

    int i;
    unsigned char tch;

    for(i=0;i < len/2;i++, h++) {
        *h = (unsigned char)((toupper(*c) - MagicNumber(toupper(*c))) <<4);
        c++;
        tch = (unsigned char)(toupper(*c) - MagicNumber(toupper(*c)));
        c++;
        *h = (unsigned char)(*h | tch);
    }
    if(len%2)
        *h = (unsigned char)((toupper(*c) - MagicNumber(toupper(*c))) <<4);

    return(0);
}

/* db 좌표 -> degree */
double convDegree(double dbCoord)
{
    int     d;
    double  m, s, degree;
    
    d = (int)dbCoord/360000;
    m = (int)(dbCoord-d*360000)/6000;
    s = (dbCoord-d*360000-m*6000)/100;
    degree = d + m/60 + s/3600;
    
    return degree;
}

/* degree-> db좌료 */
double convCoord(double degree)
{
    int     d, m;
    double  s, dbCoord;

    d = (int)degree;
    m = (int)((degree-d)*60);
    s = (((degree-d)*60)-m)*60;

    dbCoord = d*360000+m*6000+s*100;

    return (int)dbCoord;
}


int String2Octet(char *in_string, uint8_t *out_bcd)
{
	int  i, 	len;
	char tch, 	*in;
	uint8_t 	*out;

	in  = in_string;
	out = (uint8_t *)out_bcd; 
	len = strlen(in);

	if (len <= 0) {
		printf("[String2Octet] input is null string\n");
		return -1;
	} 

	for (i = 0; i < (len/2); i++, out++) {
#if 0
		*out = toupper(*in) - MagicNumber(toupper(*in++));
		tch  = toupper(*in) - MagicNumber(toupper(*in++));
#endif
		*out = toupper(*in) - MagicNumber(toupper(*in));
		in++;
		tch  = toupper(*in) - MagicNumber(toupper(*in));
		in++;
		*out |= tch << 4;
	}

	if (len % 2) {
		*out = toupper(*in) - MagicNumber(toupper(*in));
		*out |= 0x00;
		/* 홀수인 경우 마지막에 0xf0 를 넣는다 */
	} else  {
		/* 짝수인 경우 마지막에 0xff 를 넣는다 */
		*out |= 0x00;
	}

	return 0;
}

/* 'A'는 *을, 'B'는 #을 의미함 */
void Octet2String(int numdigit, unsigned char *tmp, char *term_num)
{
	int i, j, k;

	if (numdigit > 30)
		numdigit = 30;
	if (numdigit < 0)
		numdigit = 0;

	for (i = 0, j = 0; j < numdigit; i++) {
		k = 0x0F & tmp[i];
		if (k <= 9)
			term_num[j++] = k + '0';
		else if (k == 10)
			term_num[j++] = 'A';
		else if (k == 11)
			term_num[j++] = 'B';
		else if (k == 12)
			term_num[j++] = 'C';
		else
			term_num[j++] = '0';


		k = (0xF0 & tmp[i]) >> 4;
		if (k <= 9)
			term_num[j++] = k + '0';
		else if (k == 10)
			term_num[j++] = 'A';
		else if (k == 11)
			term_num[j++] = 'B';
		else if (k == 12)
			term_num[j++] = 'C';
		else
			term_num[j++] = '0';
	}
	term_num[numdigit] = 0;
}

char commlib_stringBuff[32];
char *commlib_strIp(int af, unsigned char *src)
{
	char 	buff[64];
  	struct 	in6_addr st_addr6; 

	if (af == AF_INET) {
		sprintf (commlib_stringBuff, "%d.%d.%d.%d", src[0], src[1], src[2], src[3]);
		return commlib_stringBuff;
	}

	inet_ntop (af, (void *)&src, buff, sizeof(buff));
	return commlib_stringBuff;

}

int ipinfo_str(ip_info_t *ipinfop, int size, char *str)
{
	uchar 	*p; 
	int		len = 0;

	str[0] = 0x00;

    inet_ntop(ipinfop->af, (void *)ipinfop->ip_addr, str, size);

	return strlen(str);
}

int str_to_ipinfo(char *str, int af, ip_info_t *ipInfop)
{
	int	err;

	ipInfop->af = af;

	err = inet_pton(af, str, ipInfop->ip_addr);

	return err == 1 ? 0 : -1;
}

void makeHexaString(char *data)
{
    char    tmpStr[64];
    int     i, len, wcnt = 0;
 
    len = strlen(data);
    for (i=0; i < len; i++) {
        if (isxdigit(data[i]))
            tmpStr[wcnt++] = data[i];
    }
    tmpStr[wcnt] = 0;
    sprintf(data, "%s", tmpStr);
    return;
}

void commlib_strhexa(char *str, unsigned char *src, int size)
{
	int		i, len = 0;

	for (i = 0; i < size; i++) {
		len += sprintf(&str[len], "%02x", src[i]);
	}
	return;		
}

static inline int toxdigit(char x)
{
	if (x >= '0' && x <= '9')
		return x - '0';
	if (x >= 'A' && x <= 'F')
		return x - 'A' + 10;
	if (x >= 'a' && x <= 'f')
		return x - 'a' + 10;
	return -1;
}

int str2tbcd(uint8_t *dst, const char *c)
{
	uint8_t *tp = dst;
	size_t i, len = strlen(c);

	for (i = 0; i < len/2; i++, tp++) 
	{
		*tp = toxdigit(*c);			c++;
		*tp |= (toxdigit(*c) << 4); c++;
	}

	if (len % 2)
		*tp = toxdigit(*c) | 0xF0;

	return (len / 2 + len % 2);
}

int tbcd2str(char *dst, uint8_t *h, int size)
{
	char *c = dst;
	int i;

	for (i = 0; i < size; i++) 
	{
		c[i*2]	 = (h[i] & 0x0F) + '0';
		c[i*2+1] = ((h[i] & 0xF0) >> 4) + '0';
	}

	int len = i * 2;
	if (c[len-1] == 0x3F)
		len--;

	c[len] = 0x00;

	return len;	
}

int
util_get_progname (char *argv, char *dest)
{
	int i, prog_idx = 0;
	char *p;

	char *progname = strrchr (argv, '/');
	if (! progname)
		progname = argv;
	else
		++progname;

	if ((p = strrchr (progname, '.')))
		prog_idx = atoi (p + 1);

	if (! dest)
		return prog_idx;

	char *d = dest; 
	for (i = 0; i < strlen (progname); i++)
	{
		*d = toupper (progname[i]);
		d++;
	}

	return prog_idx;
}


#define BIG_END         0
#define LIT_END         1
int getEndianType ()
{
        int i = 0x12345678;
        if (*(char *)&i == 0x12) return BIG_END;
        return LIT_END;

}

void cnvtEndianSrc (char *src, int siz)
{
    int     i;
        char    tmp[64];

        memset(tmp,     0x00,   sizeof(tmp));
        if (getEndianType() == BIG_ENDIAN)
        memcpy(tmp,     src, siz);
        else
        for (i=0; i<siz; i++) tmp[siz-i-1] = src[i];

        memcpy(src, tmp, siz);
} /** End of cnvtEndianSrc() **/

void cnvtEndianDst (char *src, char *dst, int siz)
{
    int     i;
        char    tmp[64];

        memset(tmp,     0x00,   sizeof(tmp));
        if (getEndianType() == BIG_ENDIAN)
        memcpy(tmp,     src, siz);
        else
        for (i=0; i<siz; i++) tmp[siz-i-1] = src[i];

        memcpy(dst, tmp, siz);
} 



int Remove_special_char(char* source)
{
    char* i = source;
    char* j = source;
    int  cnt = 0;

    while(*j != 0) {
        *i = *j++;
        if (*i == ' ' || *i == '\n'|| *i == '\t') {
            cnt++;
        } else {
            i++;
        }
    }
    *i = 0;
    return cnt;
}

/* 1234 -> 0x12,0x34 */
int unidirection_char2hexa(char *c, unsigned char *h, int len)
{

    int i, rlen = 0;
    unsigned char tch;


    rlen = len / 2;

    for(i=0;i < len/2;i++, h++) {
        *h = (unsigned char)((toupper(*c) - MagicNumber(toupper(*c))) <<4);
        c++;
        tch = (unsigned char)(toupper(*c) - MagicNumber(toupper(*c)));
        c++;
        *h = (unsigned char)(*h | tch);
    }

    if(len%2) {
        *h = (unsigned char)((toupper(*c) - MagicNumber(toupper(*c))) <<4);
        rlen += 1;
    }

   return rlen;
}

int unidirection_hexa2char(unsigned char *h, int hlen, char *c, int clen)
{
    register long i;
    register char tch;


    if ((hlen * 2) > clen) {
        return -1;
    }

    for(i=0 ; i < hlen; i++) {
        tch = (char)(h[i] >> 4);
        c[i*2] = (char)(tch + MagicChar(tch));
        tch = (char)(0x0F & h[i]);
        c[i*2+1] = (char)(tch + MagicChar(tch));
    }
    c[hlen*2] = 0;
    return 1;
}

char *strupr(char *inputStr)
{
	int     i;
	char    *pStr = inputStr;

	if ( inputStr == NULL ) {
		return NULL;
	}
	if ( strlen(inputStr) == 0 ) {
		return NULL;
	}

	for ( i = 0; i < strlen(inputStr); i++ ) {
		*pStr = toupper(*pStr);
		pStr++;
	}

	return inputStr;
}

