#include "keepalivelib.h"
#include "conflib.h"

#include <sys/shm.h>
#include <sys/ipc.h>

#ifndef _NEW_KEEPALIVE
T_keepalive	*keepalive;
int			keepaliveIndex;
#endif	/*** __ifndef _NEW_KEEPALIVE ***/

#ifdef _NEW_KEEPALIVE
T_keepalive *keepalive;
int			keepaliveIndex;
char		myProcName[16];
#endif	/*** __ifdef _NEW_KEEPALIVE ***/

#ifndef _NEW_KEEPALIVE
int keepalivelib_init(char *processName)
{
	char	*env;
	char	l_sysconf[64];
	char	ckeepalive[10];
	char	getBuf[256], token[CONFLIB_MAX_TOKEN_LEN];
	key_t	ikeepalive;
	int		shmid;
	FILE	*fp;

	keepaliveIndex = 0;

	if ((env = getenv(IV_HOME)) == NULL) {
		fprintf(stderr, "keepalivelib_init] not found %s environment name\n", IV_HOME);
		return -1;
	}

	sprintf(l_sysconf, "%s/%s", env, SYSCONF_FILE);
    if (conflib_getNthTokenInFileSection(l_sysconf,"SHARED_MEMORY_KEY",
        "SHM_KEEPALIVE", 1, ckeepalive) < 0) {
        fprintf(stderr,"keepalivelib_init] conflib_getNthTokenInFileSection fail[SHARED_MEMORY_KEY : SHM_KEEPALIVE] \n");
        return -1;
    }
    ikeepalive = (key_t)strtol(ckeepalive,0,0);

    if ((shmid = (int) shmget (ikeepalive, sizeof (T_keepalive), 0644|IPC_CREAT)) < 0) {
        perror ("shmget KEEPALIVE");
        fprintf(stderr,"[keepalivelib_init] shmget fail(SHM_KEEPALIVE); key=0x%x, err=%d[%s]\n", ikeepalive, errno, strerror(errno));
        return -1;
    }
    if ( (keepalive = (T_keepalive *)shmat (shmid, (char *)0, 0)) == (void*)-1 ) {
        perror ("shmat KEEPALIVE");
        fprintf(stderr,"[keepalivelib_init] shmat fail(SHM_KEEPALIVE); key=0x%x, err=%d[%s]\n", ikeepalive, errno, strerror(errno));
        return -1;
    }

    /* keepaliveIndex를 구한다 */
    if((fp = fopen(l_sysconf,"r")) == NULL) {
        fprintf(stderr,"[keepalivelib_init] fopen fail[%s]; err=%d(%s)\n", l_sysconf, errno, strerror(errno));
        return -1;
    }
    if ( conflib_seekSection (fp, "APPLICATIONS") < 0 ) {
        fprintf(stderr,"[keepalivelib_init] conflib_seekSection(APPLICATIONS) fail\n");
		fclose(fp);
        return -1;
    }

	while (fgets(getBuf,sizeof(getBuf),fp) != NULL) {
		if (getBuf[0] == '[') /* end of section */
			break;
		if (getBuf[0]=='#' || getBuf[0]=='\n') /* comment line or empty */
			continue;

		sscanf (getBuf,"%63s",token);
		if (!strcasecmp(token,processName)) {
//			fprintf(stdout, "keepaliveIndex = %d\n", keepaliveIndex);
			fclose(fp);
			return keepaliveIndex;
		}
		keepaliveIndex++;
	}
	fclose(fp);
	return -1;
}
#endif	/*** __ifndef _NEW_KEEPALIVE ***/

#ifndef _NEW_KEEPALIVE
void keepalivelib_increase()
{
	keepalive->cnt[keepaliveIndex]++;
}
#endif	/*** __ifndef _NEW_KEEPALIVE ***/

#ifdef _NEW_KEEPALIVE
int keepalivelib_init(char *processName)
{
	int 	i;
	char	*env;
	char	l_sysconf[64];
	char	ckeepalive[10];
	char	getBuf[256], token[64];
	key_t	ikeepalive;
	int		shmid;
	FILE	*fp;

	keepaliveIndex = 0;

	if ((env = getenv(IV_HOME)) == NULL) {
		fprintf(stderr, "keepalivelib_init] not found %s environment name\n", IV_HOME);
		return -1;
	}

	sprintf(l_sysconf, "%s/%s", env, SYSCONF_FILE);
    if (conflib_getNthTokenInFileSection(l_sysconf,"SHARED_MEMORY_KEY",
        "SHM_KEEPALIVE", 1, ckeepalive) < 0) {
        fprintf(stderr,"keepalivelib_init] conflib_getNthTokenInFileSection fail[SHARED_MEMORY_KEY : SHM_KEEPALIVE] \n");
        return -1;
    }
    ikeepalive = (key_t)strtol(ckeepalive,0,0);

	//printf("[%s %d] ckeepalive[%s] ikeepalive[%d]\n",__FILE__,__LINE__,ckeepalive,ikeepalive);


    if ((shmid = (int) shmget (ikeepalive, sizeof (T_keepalive), 0644|IPC_CREAT)) < 0) {
        perror ("shmget KEEPALIVE");
        fprintf(stderr,"[keepalivelib_init] shmget fail(SHM_KEEPALIVE); key=0x%x, err=%d[%s]\n", ikeepalive, errno, strerror(errno));
        return -1;
    }

//printf("[%s %d] shm[%d]\n",__FILE__,__LINE__,shmid);
#if 0
    if ( (int)(keepalive = (T_keepalive *)shmat (shmid, (char *) 0,0)) == -1 ) {
        perror ("shmat KEEPALIVE");
        fprintf(stderr,"[keepalivelib_init] shmat fail(SHM_KEEPALIVE); key=0x%x, err=%d[%s]\n", ikeepalive, errno, strerror(errno));
        return -1;
    }
#else
    if ((keepalive = (T_keepalive *)shmat (shmid, (char *) 0,0)) == (T_keepalive*)-1 ) {
        perror ("shmat KEEPALIVE");
        fprintf(stderr,"[keepalivelib_init] shmat fail(SHM_KEEPALIVE); key=0x%x, err=%d[%s]\n", ikeepalive, errno, strerror(errno));
        return -1;
    }
#endif

    /* keepaliveIndex를 구한다 */
    if((fp = fopen(l_sysconf,"r")) == NULL) {
        fprintf(stderr,"[keepalivelib_init] fopen fail[%s]; err=%d(%s)\n", l_sysconf, errno, strerror(errno));
        return -1;
    }

    if ( conflib_seekSection (fp, "APPLICATIONS") < 0 ) {
        fprintf(stderr,"[keepalivelib_init] conflib_seekSection(APPLICATIONS) fail\n");
		fclose(fp);
        return -1;
    }

	while (fgets(getBuf,sizeof(getBuf),fp) != NULL) 
	{
		if (getBuf[0] == '[') /* end of section */
			break;
		if (getBuf[0]=='#' || getBuf[0]=='\n') /* comment line or empty */
			continue;

		memset(token,0x00,sizeof(token));
		memset(myProcName,0x00,sizeof(myProcName));

		sscanf (getBuf,"%63s",token);
		if (!strcasecmp(token,processName)) 
		{
			strcpy(myProcName, processName);

			for(i=0; i<SYSCONF_MAX_APPL_NUM; i++)
			{
				if( !strcasecmp(keepalive->info[i].prcName,myProcName) )
				{
					keepalive->procCnt++;
					
					//fprintf(stdout, "1.keepaliveIndex = %s:%d=%d\n", keepalive->info[i].prcName, keepalive->procCnt, keepaliveIndex);
					
					fclose(fp);
					return keepaliveIndex;
				}
			}
			
			for(i=0; i<SYSCONF_MAX_APPL_NUM; i++)
			{
				if(keepalive->info[i].prcName[0]==0)
				{
					strcpy(keepalive->info[i].prcName, myProcName);
					keepalive->procCnt++;
					
					//fprintf(stdout, "2.keepaliveIndex = %s:%d=%d\n", keepalive->info[i].prcName, keepalive->procCnt, keepaliveIndex);
					
					fclose(fp);
					return keepaliveIndex;
				}
			}
			
			//fprintf(stdout, "keepaliveIndex = %d\n", keepaliveIndex);
		}
		keepaliveIndex++;
		memset(getBuf,	0x00,	sizeof(getBuf));
	}
	fclose(fp);
	return -1;

}
#endif	/*** __ifdef _NEW_KEEPALIVE ***/

#ifdef _NEW_KEEPALIVE
void keepalivelib_increase()
{
	//keepalive->cnt[keepaliveIndex]++;
	int i;
	/*printf("%s\n", myProcName);*/
	for(i=0; i<SYSCONF_MAX_APPL_NUM; i++)
	{
		if(!strcasecmp(keepalive->info[i].prcName, myProcName))
		{
			keepalive->info[i].cnt++;			
			/*fprintf(stderr, "[keepalivelib_increase2]keepaliveIndex = %s:%d=%d(%d)[%x]\n", keepalive->info[i].prcName, keepalive->info[i].cnt, i, keepaliveIndex,keepalive); */
			break;
		}
	}
}
#endif	/*** __ifdef _NEW_KEEPALIVE ***/


int getkeepAliveIdx(char *processName)
{
	char    *env;
	char    l_sysconf[64];
	char    getBuf[256], token[CONFLIB_MAX_TOKEN_LEN];
	FILE    *fp;
	int     blkidx = 0;

	blkidx = 0;

	if ((env = getenv(IV_HOME)) == NULL) {
		fprintf(stderr, "keepalivelib_init] not found %s environment name\n", IV_HOME);
		return -1;
	}

	sprintf(l_sysconf, "%s/%s", env, SYSCONF_FILE);

	/* blkidx를 구한다 */
	if((fp = fopen(l_sysconf,"r")) == NULL) {
		fprintf(stderr,"[keepalivelib_init] fopen fail[%s]; err=%d(%s)\n", l_sysconf, errno, strerror(errno));
		return -1;
	}

	if ( conflib_seekSection (fp, "APPLICATIONS") < 0 ) {
		fprintf(stderr,"[keepalivelib_init] conflib_seekSection(APPLICATIONS) fail\n");
		fclose(fp);
		return -1;
	}

	while (fgets(getBuf,sizeof(getBuf),fp) != NULL) {
		if (getBuf[0] == '[') /* end of section */
			break;
		if (getBuf[0]=='#' || getBuf[0]=='\n') /* comment line or empty */
			continue;

		sscanf (getBuf,"%s",token);
		if (!strcasecmp(token,processName)) {
			//fprintf(stdout, "blkidx = %d\n", blkidx);
			fclose(fp);
			return blkidx;
		}
		blkidx++;
	}

	fclose(fp);
	return -1;
}

