#include "conflib.h"
#include "sysconf.h"
#include <ctype.h>

char 	*homeEnv = NULL, l_sysconf[256], l_dataconf[256];
char 	mySysName[16];
int		mySysId;
char	gMySiteName[32];
/*------------------------------------------------------------------------------
* file을 열어 지정된 section내에서 keyword에 해당하는 라인을 찾아,
*	"=" 뒤쪽에서 n번째 token를 string에 복사한다.
* - conf 파일에는 "keyword = aaa bbb ccc ..." 의 형식으로 구성되어 있어야 한다.
* - conf 파일 한줄에 keyword = 를 제외하고 최대 16개까지 token를 기록할 수 있다.
* - 예를들어, "xxxx = aaa bbb ccc" 에서 bbb를 찾고 싶은 경우,
*	keyword=xxxx, n=2를 지정해야 한다.
------------------------------------------------------------------------------*/
int conflib_getNthTokenInFileSection (
		const char *fname,	/* configuration file name */
		char *section,	/* section name */
		char *keyword,	/* keyword -> 찾고자 하는 라인에 있는 첫번째 token name ("=" 앞에 있는 token) */
		int  n,			/* 몇번째 token을 찾을것인지 지정 ("=" 뒤에 있는 n번째 token) */
		char *string	/* 찾은 token이 저장된 string */
		)
{
	FILE	*fp;
	char    buff[1024],token[16][CONFLIB_MAX_TOKEN_LEN];
	int		lNum,lNum2;

	/* file을 연다.
	*/
	if ((fp = fopen(fname,"r")) == NULL) {
		fprintf(stderr,"[conflib_getNthTokenInFileSection] fopen fail[%s]; errno=%d(%s)\n", fname, errno, strerror(errno));
		return eCONF_NOT_EXIST_FILE;
	}

	/* section의 시작위치로 이동한다.
	*/
	if ((lNum = conflib_seekSection (fp, section)) < 0) {
		fprintf(stderr,"[conflib_getNthTokenInFileSection] not found section[%s] in file[%s]\n", section, fname);
		fclose(fp);
		return eCONF_NOT_EXIST_SECTION;
	}

	/* section내에서 keyword에 해당하는 line을 찾아 "=" 뒤쪽 내용을 buff에 복사한다.
	*/
	if ((lNum2 = conflib_getStringInSection(fp,keyword,buff)) < 0) {
		fprintf(stderr,"[conflib_getNthTokenInFileSection] not found keyword[%s]; file=%s, section=%s\n", keyword, fname, section);
		fclose(fp);
		return eCONF_NOT_EXIST_KEYWORD;
	}
	fclose(fp);
	lNum += lNum2;

	/* n번째 token을 string에 복사한다.
	*/
	if (sscanf (buff,"%63s%63s%63s%63s%63s %63s%63s%63s%63s%63s %63s%63s%63s%63s%63s %63s",
						token[0], token[1], token[2], token[3], token[4],
						token[5], token[6], token[7], token[8], token[9],
						token[10],token[11],token[12],token[13],token[14],
						token[15]) < n) {
		/* 해당 라인에 있는 token이 n개 보다 적은 경우
		*/
		fprintf(stderr,"[conflib_getNthTokenInFileSection] can't found Nth(%d) token; file=%s, section=%s, keyword=%s, lNum=%d\n", n, fname, section, keyword, lNum);
		return eCONF_NOT_EXIST_TOKEN;
	}

	/* 찾은 token을 user영역에 복사한다.
	*/
	strcpy(string,token[n-1]);

	return 1;

} /** End of conflib_getNthTokenInFileSection **/



/*------------------------------------------------------------------------------
* file을 열어 지정된 section내에서 keyword에 해당하는 라인을 찾아,
*	"=" 뒤에 몇개의 token이 있는지 return한다.
------------------------------------------------------------------------------*/
int conflib_getTokenCntInFileSection (
		char *fname,	/* configuration file name */
		char *section,	/* section name */
		char *keyword	/* keyword -> 찾고자 하는 라인에 있는 첫번째 token name ("=" 앞에 있는 token) */
		)
{
	FILE	*fp;
	char    buff[1024],token[16][CONFLIB_MAX_TOKEN_LEN];
	int		lNum,lNum2;

	/* file을 연다.
	*/
	if ((fp = fopen(fname,"r")) == NULL) {
		fprintf(stderr,"[conflib_getTokenCntInFileSection] fopen fail[%s]; errno=%d(%s)\n", fname, errno, strerror(errno));
		return eCONF_NOT_EXIST_FILE;
	}

	/* section의 시작위치로 이동한다.
	*/
	if ((lNum = conflib_seekSection (fp, section)) < 0) {
		fprintf(stderr,"[conflib_getTokenCntInFileSection] not found section[%s] in file[%s]\n", section, fname);
		fclose(fp);
		return eCONF_NOT_EXIST_SECTION;
	}

	/* section내에서 keyword에 해당하는 line을 찾아 "=" 뒤쪽 내용을 buff에 복사한다.
	*/
	if ((lNum2 = conflib_getStringInSection(fp,keyword,buff)) < 0) {
		fprintf(stderr,"[conflib_getTokenCntInFileSection] not found keyword[%s]; file=%s, section=%s\n", keyword, fname, section);
		fclose(fp);
		return eCONF_NOT_EXIST_KEYWORD;
	}
	fclose(fp);
	lNum += lNum2;

	/* token이 몇개인지 return된다.
	*/
	return (sscanf (buff,"%63s%63s%63s%63s%63s %63s%63s%63s%63s%63s %63s%63s%63s%63s%63s %63s",
						token[0], token[1], token[2], token[3], token[4],
						token[5], token[6], token[7], token[8], token[9],
						token[10],token[11],token[12],token[13],token[14],
						token[15]));

} /** End of conflib_getTokenCntInFileSection **/



/*------------------------------------------------------------------------------
* file을 열어 지정된 section내에서 keyword에 해당하는 라인을 찾아,
*	"=" 뒤쪽 string을 n개의 token으로 잘라 string에 복사한다.
* - token들이 저장될 string은 반드시 CONFLIB_MAX_TOKEN_LEN 크기로 된 array로
*	선언되어야 한다.
------------------------------------------------------------------------------*/
int conflib_getNTokenInFileSection (
		char *fname,	/* configuration file name */
		char *section,	/* section name */
		char *keyword,	/* keyword -> 찾고자 하는 라인에 있는 첫번째 token name ("=" 앞에 있는 token) */
		int  n,			/* 몇개의 token으로 잘라 낼 것인지 지정 ("=" 뒤에 있는 n개 token) */
		char string[][CONFLIB_MAX_TOKEN_LEN]	/* 찾은 token들이 저장된 string */
		)
{
	FILE	*fp;
	char    buff[1024],token[16][CONFLIB_MAX_TOKEN_LEN];
	int		i,lNum,lNum2;

	/* file을 연다.
	*/
	if ((fp = fopen(fname,"r")) == NULL) {
		fprintf(stderr,"[conflib_getNTokenInFileSection] fopen fail[%s]; errno=%d(%s)\n", fname, errno, strerror(errno));
		return eCONF_NOT_EXIST_FILE;
	}

	/* section의 시작위치로 이동한다.
	*/
	if ((lNum = conflib_seekSection (fp, section)) < 0) {
		fprintf(stderr,"[conflib_getNTokenInFileSection] not found section[%s] in file[%s]\n", section, fname);
		fclose(fp);
		return eCONF_NOT_EXIST_SECTION;
	}

	/* section내에서 keyword에 해당하는 line을 찾아 "=" 뒤쪽 내용을 buff에 복사한다.
	*/
	if ((lNum2 = conflib_getStringInSection(fp,keyword,buff)) < 0) {
		fprintf(stderr,"[conflib_getNTokenInFileSection] not found keyword[%s]; file=%s, section=%s\n", keyword, fname, section);
		fclose(fp);
		return eCONF_NOT_EXIST_KEYWORD;
	}
	fclose(fp);
	lNum += lNum2;

	/* n번째 token을 string에 복사한다.
	*/
	if (sscanf (buff,"%63s%63s%63s%63s%63s %63s%63s%63s%63s%63s %63s%63s%63s%63s%63s %63s",
						token[0], token[1], token[2], token[3], token[4],
						token[5], token[6], token[7], token[8], token[9],
						token[10],token[11],token[12],token[13],token[14],
						token[15]) < n) {
		/* 해당 라인에 있는 token이 n개 보다 적은 경우
		*/
		fprintf(stderr,"[conflib_getNTokenInFileSection] can't found N(%d) token; file=%s, section=%s, keyword=%s, lNum=%d\n", n, fname, section, keyword, lNum);
		return eCONF_NOT_EXIST_TOKEN;
	}

	/* 잘라낸 token들을 user영역에 복사한다.
	*/
	for (i=0; i<n; i++) {
		strcpy(string[i],token[i]);
	} 
	return 1;

} /** End of conflib_getNTokenInFileSection **/



/*------------------------------------------------------------------------------
* file을 열어 지정된 section내에서 keyword에 해당하는 라인을 찾아,
*	"=" 뒤에 있는 내용을 string에 모두 복사한다.
------------------------------------------------------------------------------*/
int conflib_getStringInFileSection (
		char *fname,	/* configuration file name */
		char *section,	/* section name */
		char *keyword,	/* keyword -> 찾고자 하는 라인에 있는 첫번째 token name ("=" 앞에 있는 token) */
		char *string	/* 찾은 내용이 저장된 string */
		)
{
	FILE    *fp;

	/* file을 연다.
	*/
	if ((fp = fopen (fname,"r")) == NULL) {
		fprintf(stderr,"[conflib_getStringInFileSection] fopen fail[%s]; errno=%d(%s)\n",fname,errno,strerror(errno));
		return eCONF_NOT_EXIST_FILE;
	}

	/* section의 시작위치로 이동한다.
	*/
	if (conflib_seekSection(fp,section) < 0) {
		fprintf(stderr,"[conflib_getStringInFileSection] not found section[%s] in file[%s]\n",section,fname);
		fclose(fp);
		return eCONF_NOT_EXIST_SECTION;
	}

	/* section내에서 keyword에 해당하는 line을 찾아 "=" 뒤쪽 내용을 string에 복사한다.
	*/
	if (conflib_getStringInSection(fp,keyword,string) < 0) {
		fprintf(stderr,"[conflib_getStringInFileSection] not found keyword[%s]; file=%s, section=%s\n", keyword, fname, section);
		fclose(fp);
		return eCONF_NOT_EXIST_KEYWORD;
	}

	fclose(fp);
	return 1;

} /** End of conflib_getStringInFileSection **/



/*------------------------------------------------------------------------------
* fopen으로 열려있는 파일에서 지정된 section으로 fp를 이동한다.
* - 파일의 처음부터 한줄씩 읽어 지정된 section을 찾고, line_number를 return한다.
* - conf 파일에서 section의 구분은 "[xxxx]"가 있는 줄부터 시작되어 다음 section의
*	시작까지이다.
------------------------------------------------------------------------------*/
int conflib_seekSection (
		FILE *fp,		/* file pointer */
		char *section	/* section name */
		)
{
	char    getBuf[1024];
	int		lNum=0;

	rewind(fp);
	while (fgets(getBuf,sizeof(getBuf),fp) != NULL) {
		lNum++;
		if (getBuf[0] != '[')
			continue;
		if (strstr(getBuf,section)) {
			return lNum;
		}
	}
	fprintf(stderr,"[conflib_seekSection] not found section[%s]\n", section);
	return eCONF_NOT_EXIST_SECTION;

} /** End of conflib_seekSection **/



/*------------------------------------------------------------------------------
* fopen으로 열려있는 파일에서 현재 fp위치에서부터 현재 section내에서 지정된 keyword에
*	해당하는 line을 찾아, "=" 뒤쪽 내용을 string에 복사한다.
* - 검색한 line_number를 return한다.
------------------------------------------------------------------------------*/
int conflib_getStringInSection (
		FILE *fp,		/* file pointer */
		char *keyword,	/* keyword -> 찾고자 하는 라인에 있는 첫번째 token name ("=" 앞에 있는 token) */
		char *string	/* 찾은 내용이 저장된 string */
		)
{
	char    getBuf[1024],token[CONFLIB_MAX_TOKEN_LEN],*next;
	int		lNum=0;

	/* section내에서 keyword에 해당하는 line을 찾아 "=" 뒤쪽 내용을 string에 복사한다.
	*/
	while (fgets(getBuf,sizeof(getBuf),fp) != NULL) {
		lNum++;
		if (getBuf[0] == '[') /* end of section */
			break;
		if (getBuf[0]=='#' || getBuf[0]=='\n') /* comment line or empty */
			continue;

		sscanf (getBuf,"%63s",token);
		if (!strcmp(token, keyword)) {
			strtok_r(getBuf, "=", &next);
			for (; isspace(*next); next++)  /* 앞쪽 white-space를 없앤다.*/
			strcpy (string, next);
			return lNum;
		}
	}
	fprintf(stderr,"[conflib_getStringInSection] not found keyword[%s]\n", keyword);
	return eCONF_NOT_EXIST_KEYWORD;

} /** End of conflib_getStringInSection **/

/* homeEnv, l_sysconf, mySysName 을 설정하는 함수 */
int conflib_initConfigData(void)
{
    char    *env;
    homeEnv = getenv(IV_HOME);
    if (homeEnv == NULL) {
        fprintf (stderr, "[conflib_initConfigData] Not Found Environment Name [%s]\n", IV_HOME);
        return eCONF_NOT_FOUND_ENV;
    }
    sprintf(l_sysconf, "%s/%s", homeEnv, SYSCONF_FILE);
	sprintf(l_dataconf, "%s/%s", homeEnv, DATACONF_FILE);
    env = getenv(MY_SYS_NAME);
    if (env == NULL) {
        fprintf (stderr, "[conflib_initConfigData] Not Found Environment Name [%s]\n", MY_SYS_NAME);
        return eCONF_NOT_FOUND_ENV;
    }
    sprintf(mySysName, "%s", env);

	/* 2011.09.22 */
    env = getenv(SYS_MP_ID);
    if (env == NULL) {
        fprintf (stderr, "[conflib_initConfigData] Not Found Environment Name [%s]\n", SYS_MP_ID);
        return eCONF_NOT_FOUND_ENV;
    }
	mySysId = atoi(env);

    return 1;
}	/** End of conflib_initConfigData **/


const char*
conflib_get_home (void)
{
	if (homeEnv)
		return homeEnv;

    homeEnv = getenv(IV_HOME);

	return homeEnv;
}

char*
conflib_get_sysconf (void)
{
	if (l_sysconf[0])
	   return l_sysconf;	

	const char *env = conflib_get_home ();
    if (env == NULL) {
        fprintf (stderr, "[conflib_initConfigData] Not Found Environment Name [%s]\n", IV_HOME);
        return NULL;
    }

    sprintf(l_sysconf, "%s/%s", env, SYSCONF_FILE);

	return l_sysconf;
}

int conflib_get_sysconf_contents (char *section, char *keyword, int  n, char *string)
{
	const char *sysconf = conflib_get_sysconf ();

	if (sysconf == NULL)
		return -1;

	return conflib_getNthTokenInFileSection (sysconf, section, keyword, n, string);
}

int conflib_get_token (const char *file, char *section, char *keyword, char *buf)
{
	return conflib_getNthTokenInFileSection (file, section, keyword, 1, buf);
}

int conflib_get_contents (char *file, char *section, char *keyword, char *buf)
{
	return conflib_getStringInFileSection (file, section, keyword, buf);
}

int conflib_get_app_num (const char *app_name)
{
	FILE	*fp;
	int		num = 0;
	char	getBuf[256], token[64];
	const char *sysconf = conflib_get_sysconf ();
	if (! sysconf)
	{
		fprintf(stderr,"\n sysconf is null\n\n");
		return -1;
	}

	if ((fp = fopen(sysconf,"r")) == NULL) {
		fprintf(stderr,"\n fopen fail[%s]; err=%d(%s)\n\n", sysconf, errno, strerror(errno));
		return -1;
	}

	if (conflib_seekSection(fp,"APPLICATIONS") < 0) {
		fprintf(stderr,"\n not found section[APPLICATIONS] in file[%s]\n\n", sysconf);
		return -1;
	}

	while ((fgets(getBuf,sizeof(getBuf),fp) != NULL)) 
	{
		if (getBuf[0] == '[') /* end of section */
			break;
		if (getBuf[0]=='#' || getBuf[0]=='\n') /* comment line or empty */
			continue;

		memset (token, 0x00, sizeof (token));
		sscanf(getBuf,"%63s",token);

		if (! strncmp (token, app_name, strlen (app_name)))
			num++;
	}

	fclose(fp);

	return num;

} 

