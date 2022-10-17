/*
 * FILE : mmclib.h
 */
#ifndef	__MMCLIB_H__
#define	__MMCLIB_H__

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "comm_msgtypes.h"
#include "conflib.h"
#include "loglib.h"

#define MAX_MMC_HDLR	1024

extern char myAppName[COMM_MAX_NAME_LEN];
extern int ixpcQid;

int mmclib_initMmc(MmcHdlrVector* mmcHdlrVector);
void mmclib_handleMmcReq (IxpcQMsgType *rxIxpcMsg);
int mmclib_sendMmcResult (IxpcQMsgType *rxIxpcMsg, char *buff, char resCode,
		char contFlag, unsigned short extendTime, char segFlag, char seqNo);

int mmclib_printFailureHeader(char *buff, char *reason);
int mmclib_printSuccessHeader(char *buff);
int mmclib_getMmlPara_IDX(IxpcQMsgType *rxIxpcMsg, char *paraName);
int mmclib_getMmlPara_INT(IxpcQMsgType *rxIxpcMsg, char *paraName);
int mmclib_getMmlPara_HEXA(IxpcQMsgType *rxIxpcMsg, char *paraName);
int mmclib_getMmlPara_STR(IxpcQMsgType *rxIxpcMsg, char *paraName, char *buff);

#endif

