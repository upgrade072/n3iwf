#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "typedef.h"
#include "comm_msgq.h"
#include "conflib.h"


static char clr_buff[MAX_MSGQ_LEN]; 

static int msgq_id = -1;
static char msgq_app[32] = {};

int commlib_msgget(int key, int initFlag);

int commlib_getMsgQKey(const char *l_sysconf, char *appName, int *key, int isPrimary)
{
    char 	tmp[64];
	int		qfield;

	if( isPrimary ) 
		qfield = 3;
	else
		qfield = 4;

    if( conflib_getNthTokenInFileSection (l_sysconf, "[APPLICATIONS]", appName, qfield, tmp)<0 )
        return(-1);

    *key = (int )strtol(tmp, 0, 0);
    return 0;
}

int commlib_getMsgQKeyNth(const char *l_sysconf, char *appName, int *key, int nth /* nth = 0, 1, 2  ... */)
{
    char 	tmp[64];
	int		qfield = nth + 3;

    if( conflib_getNthTokenInFileSection (l_sysconf, "[APPLICATIONS]", appName, qfield, tmp)<0 )
        return(-1);

    *key = (int )strtol(tmp, 0, 0);
    return 0;
}

int commlib_crteMsgQ (const char *l_sysconf, char *appName, const int initFlag)
{
	int	key;

	if (l_sysconf == NULL || l_sysconf[0] == 0x00)
	{
		fprintf (stderr, "[%s.%d] l_sysconfig:%p \n", __FILE__, __LINE__, l_sysconf);
		return -1;
	}

	if( commlib_getMsgQKey(l_sysconf, appName, &key, TRUE)<0 ) {
		fprintf (stderr, "Not Found %s's MSGQ Key\n", appName);
		return -1;
	}

	return commlib_msgget(key, initFlag);

}

int commlib_crteSecondMsgQ (const char *l_sysconf, char *appName, const int initFlag)
{
	int	key;

	if( commlib_getMsgQKey(l_sysconf, appName, &key, FALSE)<0 ) {
		fprintf (stderr, "[%s] Not Found %s's Second MSGQ Key\n", __func__, appName);
		return -1;
	}

	return commlib_msgget(key, initFlag);

}

int commlib_crteNthMsgQ (const char *l_sysconf, char *appName, int nth, const int initFlag)
{
	int		qfield = 3+nth;
	long	key;
	char	tmp[64];

    if (conflib_getNthTokenInFileSection (l_sysconf, "[APPLICATIONS]", appName, qfield, tmp) < 0)
        return(-1);

	if (!strcmp(tmp, "NULL"))
		return -1;

    key = strtol(tmp, 0, 0);

	return commlib_msgget(key, initFlag);

}


int commlib_msgget(int key, int initFlag)
{
	int qid;

	/**if ((msg_qid = msgget (key, IPC_CREAT|IPC_EXCL|0666)) < 0)**/
	if ((qid = msgget (key, IPC_CREAT|0666)) < 0)
	{
		if (errno != EEXIST)
		{
			fprintf (stderr, "Message Queue create failed!!!(errno=%d)\n", errno);
			return -1;
		}

		fprintf (stderr, "Message Queue Already Exist(create failed)!!!(errno=%d)\n", errno);
		qid = msgget (key, 0);

	}
	if (initFlag == 1) {
		while (msgrcv (qid, clr_buff, MAX_MSGQ_LEN, 0, IPC_NOWAIT) > 0) {
			;
		}
		//fprintf (stderr, "Queue Cleared !!!\n");
	}

	return (int)qid;
}


int commlib_crteMsgQByKey (const int qkey, const int initFlag)
{
	int qid;

	if ((qid = msgget (qkey, IPC_CREAT|0666)) < 0)
	{
		if (errno != EEXIST)
		{
			fprintf (stderr, "Message Queue create failed!!!(errno=%d)\n", errno);
			return -1;
		}

		fprintf (stderr, "Message Queue Already Exist(create failed)!!!(errno=%d)\n", errno);
		qid = msgget (qkey, 0);

	}
	if (initFlag == 1) {
		while (msgrcv (qid, clr_buff, MAX_MSGQ_LEN, 0, IPC_NOWAIT) > 0) {
			;
		}
		//printf (stderr, "Queue Cleared !!!\n");
	}

	return (int)qid;
}

int
msgq_get_id (void)
{
	return msgq_id;
}

int msgq_create (char *app_name, int clear_flag)
{
	const char *sysconf = conflib_get_sysconf ();

	if (sysconf)
		return commlib_crteMsgQ (sysconf, app_name, clear_flag);

	return -1;
}


inline void
msgq_clear (int msgq_id)
{
	while (msgrcv (msgq_id, clr_buff, sizeof (clr_buff), 0, IPC_NOWAIT) > 0) 
	{ ; }
}

int 
msgq_init (char *app_name, int clear_flag)
{
	const char *sysconf = conflib_get_sysconf (); 

	if (! sysconf)
	{
		fprintf (stderr, "[%s] <%s> sysconf(H'%p) is null\n", __FILE__, app_name, sysconf);
		return -1;
	}

	snprintf (msgq_app, sizeof (msgq_app), "%s", app_name);

	if (msgq_id < 0)
	{
		msgq_id = commlib_crteMsgQ (sysconf, app_name, clear_flag);
		return msgq_id;
	}

	if (! clear_flag)
		return msgq_id;

	msgq_clear (msgq_id);

	return msgq_id;
}

int
msgq_send (int msgq_id, char *msg, int size)
{
	if (msgsnd (msgq_id, msg, size, IPC_NOWAIT) < 0)
		return -errno;

	return 0;
}

int
msgq_recv (char *msg, int size)
{
	if (msgq_id < 0)
		return -1;

	int nbytes = msgrcv (msgq_id, msg, size, 0, IPC_NOWAIT);
    if (nbytes < 0)
	{
		if (errno != ENOMSG) 
			return -errno;
		return 0;
	}

	return nbytes;
}

