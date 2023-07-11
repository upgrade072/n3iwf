#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <typedef.h>
#include <comm_msgtypes.h>
#include <comm_msgq.h>
#include "daemon.h"

static int ixpc_qid = 0;
static char proc_name[32] = {};

int
ixpc_que_get_id (void)
{
	if (ixpc_qid)
		return ixpc_qid;

	ixpc_qid = msgq_create (PROC_NAME_IXPC, FALSE);
	if (ixpc_qid < 0)
	{
		fprintf(stderr, "[%s.%d] crte-msgq failure (ixpc)\n", __FILE__, __LINE__);
		return -1;
	}

	return ixpc_qid;
}


int
ixpc_que_send_ex (struct general_qmsg *snd, int mtype, int msgid, char *dest, int length, 
		int seg_flag, int seqno)
{       
	struct ixpc_qmsg *ixpc = (struct ixpc_qmsg *)snd->body;
	struct ixpc_header *head = &ixpc->head;
	const char *source = proc_name;

	IXPC_SET_HEADER (head, mtype, msgid, source, dest, length);

	head->segFlag	= seg_flag;
	head->seqNo		= seqno;

	if (msgsnd (ixpc_que_get_id (), 
				(char *)snd, 
				GEN_QMSG_LEN (IXPC_QMSG_LEN (head->bodyLen)) , 
				IPC_NOWAIT) < 0)
	{
		fprintf (stderr, "msgq send fail. qid(%d), msg(%d:%d) (%s->%s) len:%d/%ld errno:%d(%s)\n", 
				ixpc_que_get_id (), mtype, msgid, source, dest, 
				head->bodyLen, GEN_QMSG_LEN (IXPC_QMSG_LEN (head->bodyLen)),
				errno, strerror (errno));

		return -1;
	}

	return 0;
}

int
ixpc_que_send (struct general_qmsg *snd, int mtype, int msgid, char *dest, int length)
{       
	return ixpc_que_send_ex (snd, mtype, msgid, dest, length, 0, 0);
}

int
ixpc_que_send_to_cond (int mtype, int msgid, char *text)
{
	struct general_qmsg qmsg, *snd = &qmsg;
	struct ixpc_qmsg *ixpc = (struct ixpc_qmsg *)snd->body;

	int len = snprintf (ixpc->body, sizeof (ixpc->body), "%s", text);
	return ixpc_que_send (snd, mtype, msgid, PROC_NAME_COND, len);
}

int
ixpc_que_init (char *app_name)
{
	if (ixpc_que_get_id () < 0)
		return -1;

	snprintf (proc_name, sizeof (proc_name), "%s", app_name);

	return 0;
}

