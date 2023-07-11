#ifndef __msgtrclib_h__
#define __msgtrclib_h__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define TRC_CHECK_START		1
#define TRC_CHECK_CONTINUE	2

typedef struct  {
#define MAX_TRC_LEN            20
	char            trcNum[MAX_TRC_LEN+1];
#define MAX_TRC_SUB_LEN		120
	char			trcSub[5][MAX_TRC_SUB_LEN+1];
	int 		    testRule;
#define TRC_SIMPLE	1
#define TRC_MIDDLE	2
#define TRC_DETAIL	3
#define TRC_RULE_OFFSET	10
	int				trc_level;
	time_t          expiredTime;
	int				trcFlag;
} TraceInfo;
 
#define MAX_TRACE_LIST     40
typedef struct {
	int				listCnt;
    TraceInfo		info[MAX_TRACE_LIST];
} TraceList;

enum trace_node {
	TRACE_NODE_NONE		= 0,
	TRACE_NODE_UE		= 1,
	TRACE_NODE_UP		= 2,
	TRACE_NODE_CP		= 3,
	TRACE_NODE_AMF      = 4,
};
 
typedef struct {
    // unsigned int opCode; //해당 Trace Message의 Operation Code ( Req/Ack별도 정의 )
    // 0 ~    99 : 2G/3G MAP
    // 100 ~ 199 : IMS MAP 
    // 200 ~ 299 : LTE MAP
    // opCode를 int 형에서 chsr[] 로 변경 JKLEE 2011/03/22 
#define MAX_OPCODE_LEN      64
    char        opCode[MAX_OPCODE_LEN];
 
#define DIR_SEND        1
#define DIR_RECEIVE     2

	unsigned int direction;
	unsigned int originSysType;     // (from 노드 타입),  
	unsigned int destSysType;       // (to 노드 타입)
#define TRC_TIME_MAX_LEN 32
#define MAX_KEY_LEN      128		// 64 --> 128 for 5G 
	char         trcTime[TRC_TIME_MAX_LEN];    // 1/1000초 예> ‘2011-03-14 13:00:33.339’
	char         primaryKey[MAX_KEY_LEN];      // 64  : MDN 예> ‘01029210000’,IMS Public User Identity : KEY 예> ‘sip:username@ktimssvc.kt.com’
	char         secondaryKey[MAX_KEY_LEN];    // 64  : optional임[IMSI] 예> ‘450081234568888’ 
#define TRC_MSG_BODY_MAX_LEN    (15 * 1024) // 7500 --> 15360
	char        trcMsg[TRC_MSG_BODY_MAX_LEN]; //Trace Message Body 내용  
# define  TRCMSG_INIT_MSG           0
# define  TRCMSG_ADD_MSG            1
	char        trcMsgType;
} TraceMsgInfo;


typedef struct {
	char         opCode[MAX_OPCODE_LEN];
	unsigned int direction;
	unsigned int originSysType;  
	unsigned int destSysType; 
	char         trcTime[TRC_TIME_MAX_LEN]; 
	char         primaryKey[MAX_KEY_LEN];
	char         secondaryKey[MAX_KEY_LEN];
} TraceMsgHead;


extern TraceList	*trcList;

extern int msgtrclib_init(void);
extern int msgtrclib_traceOk(int type, int cmd, char *trcKey, char *subKey);
extern void msgtrclib_makeTraceMsg(char *msg, char *key, char* addHead, char *data);
extern void msgtrclib_sendTraceMsg(TraceMsgHead *head, char *msg);

extern int msgtrclib_addTraceList(char *trcNum, time_t expiredTime, char *errStr, int, int);
extern int msgtrclib_delTraceList(char *trcNum, char *errStr);
extern void msgtrclib_checkTrcTime(time_t now);
extern void msgtrclib_sendAlarmMsg(int msgId_almCode, int almLevel, char *almDesc, char *almInfo);

#endif
