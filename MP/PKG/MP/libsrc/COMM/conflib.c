#include "conflib.h"
#include "sysconf.h"
#include <ctype.h>

char 	*homeEnv = NULL, l_sysconf[256], l_dataconf[256];
char 	mySysName[16];
int		mySysId;
char	gMySiteName[32];
/*------------------------------------------------------------------------------
* file�� ���� ������ section������ keyword�� �ش��ϴ� ������ ã��,
*	"=" ���ʿ��� n��° token�� string�� �����Ѵ�.
* - conf ���Ͽ��� "keyword = aaa bbb ccc ..." �� �������� �����Ǿ� �־�� �Ѵ�.
* - conf ���� ���ٿ� keyword = �� �����ϰ� �ִ� 16������ token�� ����� �� �ִ�.
* - �������, "xxxx = aaa bbb ccc" ���� bbb�� ã�� ���� ���,
*	keyword=xxxx, n=2�� �����ؾ� �Ѵ�.
------------------------------------------------------------------------------*/
int conflib_getNthTokenInFileSection (
		const char *fname,	/* configuration file name */
		char *section,	/* section name */
		char *keyword,	/* keyword -> ã���� �ϴ� ���ο� �ִ� ù��° token name ("=" �տ� �ִ� token) */
		int  n,			/* ���° token�� ã�������� ���� ("=" �ڿ� �ִ� n��° token) */
		char *string	/* ã�� token�� ����� string */
		)
{
	FILE	*fp;
	char    buff[1024],token[16][CONFLIB_MAX_TOKEN_LEN];
	int		lNum,lNum2;

	/* file�� ����.
	*/
	if ((fp = fopen(fname,"r")) == NULL) {
		fprintf(stderr,"[conflib_getNthTokenInFileSection] fopen fail[%s]; errno=%d(%s)\n", fname, errno, strerror(errno));
		return eCONF_NOT_EXIST_FILE;
	}

	/* section�� ������ġ�� �̵��Ѵ�.
	*/
	if ((lNum = conflib_seekSection (fp, section)) < 0) {
		fprintf(stderr,"[conflib_getNthTokenInFileSection] not found section[%s] in file[%s]\n", section, fname);
		fclose(fp);
		return eCONF_NOT_EXIST_SECTION;
	}

	/* section������ keyword�� �ش��ϴ� line�� ã�� "=" ���� ������ buff�� �����Ѵ�.
	*/
	if ((lNum2 = conflib_getStringInSection(fp,keyword,buff)) < 0) {
		fprintf(stderr,"[conflib_getNthTokenInFileSection] not found keyword[%s]; file=%s, section=%s\n", keyword, fname, section);
		fclose(fp);
		return eCONF_NOT_EXIST_KEYWORD;
	}
	fclose(fp);
	lNum += lNum2;

	/* n��° token�� string�� �����Ѵ�.
	*/
	if (sscanf (buff,"%63s%63s%63s%63s%63s %63s%63s%63s%63s%63s %63s%63s%63s%63s%63s %63s",
						token[0], token[1], token[2], token[3], token[4],
						token[5], token[6], token[7], token[8], token[9],
						token[10],token[11],token[12],token[13],token[14],
						token[15]) < n) {
		/* �ش� ���ο� �ִ� token�� n�� ���� ���� ���
		*/
		fprintf(stderr,"[conflib_getNthTokenInFileSection] can't found Nth(%d) token; file=%s, section=%s, keyword=%s, lNum=%d\n", n, fname, section, keyword, lNum);
		return eCONF_NOT_EXIST_TOKEN;
	}

	/* ã�� token�� user������ �����Ѵ�.
	*/
	strcpy(string,token[n-1]);

	return 1;

} /** End of conflib_getNthTokenInFileSection **/



/*------------------------------------------------------------------------------
* file�� ���� ������ section������ keyword�� �ش��ϴ� ������ ã��,
*	"=" �ڿ� ��� token�� �ִ��� return�Ѵ�.
------------------------------------------------------------------------------*/
int conflib_getTokenCntInFileSection (
		char *fname,	/* configuration file name */
		char *section,	/* section name */
		char *keyword	/* keyword -> ã���� �ϴ� ���ο� �ִ� ù��° token name ("=" �տ� �ִ� token) */
		)
{
	FILE	*fp;
	char    buff[1024],token[16][CONFLIB_MAX_TOKEN_LEN];
	int		lNum,lNum2;

	/* file�� ����.
	*/
	if ((fp = fopen(fname,"r")) == NULL) {
		fprintf(stderr,"[conflib_getTokenCntInFileSection] fopen fail[%s]; errno=%d(%s)\n", fname, errno, strerror(errno));
		return eCONF_NOT_EXIST_FILE;
	}

	/* section�� ������ġ�� �̵��Ѵ�.
	*/
	if ((lNum = conflib_seekSection (fp, section)) < 0) {
		fprintf(stderr,"[conflib_getTokenCntInFileSection] not found section[%s] in file[%s]\n", section, fname);
		fclose(fp);
		return eCONF_NOT_EXIST_SECTION;
	}

	/* section������ keyword�� �ش��ϴ� line�� ã�� "=" ���� ������ buff�� �����Ѵ�.
	*/
	if ((lNum2 = conflib_getStringInSection(fp,keyword,buff)) < 0) {
		fprintf(stderr,"[conflib_getTokenCntInFileSection] not found keyword[%s]; file=%s, section=%s\n", keyword, fname, section);
		fclose(fp);
		return eCONF_NOT_EXIST_KEYWORD;
	}
	fclose(fp);
	lNum += lNum2;

	/* token�� ����� return�ȴ�.
	*/
	return (sscanf (buff,"%63s%63s%63s%63s%63s %63s%63s%63s%63s%63s %63s%63s%63s%63s%63s %63s",
						token[0], token[1], token[2], token[3], token[4],
						token[5], token[6], token[7], token[8], token[9],
						token[10],token[11],token[12],token[13],token[14],
						token[15]));

} /** End of conflib_getTokenCntInFileSection **/



/*------------------------------------------------------------------------------
* file�� ���� ������ section������ keyword�� �ش��ϴ� ������ ã��,
*	"=" ���� string�� n���� token���� �߶� string�� �����Ѵ�.
* - token���� ����� string�� �ݵ�� CONFLIB_MAX_TOKEN_LEN ũ��� �� array��
*	����Ǿ�� �Ѵ�.
------------------------------------------------------------------------------*/
int conflib_getNTokenInFileSection (
		char *fname,	/* configuration file name */
		char *section,	/* section name */
		char *keyword,	/* keyword -> ã���� �ϴ� ���ο� �ִ� ù��° token name ("=" �տ� �ִ� token) */
		int  n,			/* ��� token���� �߶� �� ������ ���� ("=" �ڿ� �ִ� n�� token) */
		char string[][CONFLIB_MAX_TOKEN_LEN]	/* ã�� token���� ����� string */
		)
{
	FILE	*fp;
	char    buff[1024],token[16][CONFLIB_MAX_TOKEN_LEN];
	int		i,lNum,lNum2;

	/* file�� ����.
	*/
	if ((fp = fopen(fname,"r")) == NULL) {
		fprintf(stderr,"[conflib_getNTokenInFileSection] fopen fail[%s]; errno=%d(%s)\n", fname, errno, strerror(errno));
		return eCONF_NOT_EXIST_FILE;
	}

	/* section�� ������ġ�� �̵��Ѵ�.
	*/
	if ((lNum = conflib_seekSection (fp, section)) < 0) {
		fprintf(stderr,"[conflib_getNTokenInFileSection] not found section[%s] in file[%s]\n", section, fname);
		fclose(fp);
		return eCONF_NOT_EXIST_SECTION;
	}

	/* section������ keyword�� �ش��ϴ� line�� ã�� "=" ���� ������ buff�� �����Ѵ�.
	*/
	if ((lNum2 = conflib_getStringInSection(fp,keyword,buff)) < 0) {
		fprintf(stderr,"[conflib_getNTokenInFileSection] not found keyword[%s]; file=%s, section=%s\n", keyword, fname, section);
		fclose(fp);
		return eCONF_NOT_EXIST_KEYWORD;
	}
	fclose(fp);
	lNum += lNum2;

	/* n��° token�� string�� �����Ѵ�.
	*/
	if (sscanf (buff,"%63s%63s%63s%63s%63s %63s%63s%63s%63s%63s %63s%63s%63s%63s%63s %63s",
						token[0], token[1], token[2], token[3], token[4],
						token[5], token[6], token[7], token[8], token[9],
						token[10],token[11],token[12],token[13],token[14],
						token[15]) < n) {
		/* �ش� ���ο� �ִ� token�� n�� ���� ���� ���
		*/
		fprintf(stderr,"[conflib_getNTokenInFileSection] can't found N(%d) token; file=%s, section=%s, keyword=%s, lNum=%d\n", n, fname, section, keyword, lNum);
		return eCONF_NOT_EXIST_TOKEN;
	}

	/* �߶� token���� user������ �����Ѵ�.
	*/
	for (i=0; i<n; i++) {
		strcpy(string[i],token[i]);
	} 
	return 1;

} /** End of conflib_getNTokenInFileSection **/



/*------------------------------------------------------------------------------
* file�� ���� ������ section������ keyword�� �ش��ϴ� ������ ã��,
*	"=" �ڿ� �ִ� ������ string�� ��� �����Ѵ�.
------------------------------------------------------------------------------*/
int conflib_getStringInFileSection (
		char *fname,	/* configuration file name */
		char *section,	/* section name */
		char *keyword,	/* keyword -> ã���� �ϴ� ���ο� �ִ� ù��° token name ("=" �տ� �ִ� token) */
		char *string	/* ã�� ������ ����� string */
		)
{
	FILE    *fp;

	/* file�� ����.
	*/
	if ((fp = fopen (fname,"r")) == NULL) {
		fprintf(stderr,"[conflib_getStringInFileSection] fopen fail[%s]; errno=%d(%s)\n",fname,errno,strerror(errno));
		return eCONF_NOT_EXIST_FILE;
	}

	/* section�� ������ġ�� �̵��Ѵ�.
	*/
	if (conflib_seekSection(fp,section) < 0) {
		fprintf(stderr,"[conflib_getStringInFileSection] not found section[%s] in file[%s]\n",section,fname);
		fclose(fp);
		return eCONF_NOT_EXIST_SECTION;
	}

	/* section������ keyword�� �ش��ϴ� line�� ã�� "=" ���� ������ string�� �����Ѵ�.
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
* fopen���� �����ִ� ���Ͽ��� ������ section���� fp�� �̵��Ѵ�.
* - ������ ó������ ���پ� �о� ������ section�� ã��, line_number�� return�Ѵ�.
* - conf ���Ͽ��� section�� ������ "[xxxx]"�� �ִ� �ٺ��� ���۵Ǿ� ���� section��
*	���۱����̴�.
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
* fopen���� �����ִ� ���Ͽ��� ���� fp��ġ�������� ���� section������ ������ keyword��
*	�ش��ϴ� line�� ã��, "=" ���� ������ string�� �����Ѵ�.
* - �˻��� line_number�� return�Ѵ�.
------------------------------------------------------------------------------*/
int conflib_getStringInSection (
		FILE *fp,		/* file pointer */
		char *keyword,	/* keyword -> ã���� �ϴ� ���ο� �ִ� ù��° token name ("=" �տ� �ִ� token) */
		char *string	/* ã�� ������ ����� string */
		)
{
	char    getBuf[1024],token[CONFLIB_MAX_TOKEN_LEN],*next;
	int		lNum=0;

	/* section������ keyword�� �ش��ϴ� line�� ã�� "=" ���� ������ string�� �����Ѵ�.
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
			for (; isspace(*next); next++)  /* ���� white-space�� ���ش�.*/
			strcpy (string, next);
			return lNum;
		}
	}
	fprintf(stderr,"[conflib_getStringInSection] not found keyword[%s]\n", keyword);
	return eCONF_NOT_EXIST_KEYWORD;

} /** End of conflib_getStringInSection **/

/* homeEnv, l_sysconf, mySysName �� �����ϴ� �Լ� */
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

