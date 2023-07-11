#ifndef __COMM_ALM_H__
#define __COMM_ALM_H__

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "comm_msgtypes.h"
#include "conflib.h"
#include "loglib.h"

#define ALMCODE_SFM_SYSTEM          		9999
#define ALMCODE_SFM_CPU_USAGE       		1010
#define ALMCODE_SFM_MEMORY_USAGE    		1020
#define ALMCODE_SFM_DISK_USAGE      		1030
#define ALMCODE_SFM_LAN             		1040
#define ALMCODE_SFM_PROCESS         		1050
#define ALMCODE_SFM_QUEUE           		1060
#define ALMCODE_SFM_HW              		1070
#define ALMCODE_SFM_DB_PROCESS      		1100

#define ALMCODE_SFM_EVENT_ALARM_START     	2000
#define ALMCODE_SFM_EVENT_ALARM_END       	5000
#define ALMCODE_EVENT_QUEUE_CLEAR         	2000
#define ALMCODE_EVENT_FAILOVER            	2010
#define ALMCODE_EVENT_NTP                 	2020

#define ALMCODE_EVENT_NOTALLOWED_LOGIN    	2320
#define ALMCODE_EVENT_CKSUM                 2310
#define ALMCODE_EVENT_NOTALLOWED_IP         2340

#define STSCODE_OVERLOAD_CONTROL            2900    

#define ALMCODE_EVENT_FTP_PUT_FAIL			3000

extern char myAppName[COMM_MAX_NAME_LEN];
extern int  ixpcQid;

extern int almlib_sentEventAlarm(int almLevel, int almCode, char *text, char *desc);
extern int almlib_sentPemEventAlarm(int almLevel, int almCode, char *text, char *desc, char *srcSysName, char *appName);

#endif
